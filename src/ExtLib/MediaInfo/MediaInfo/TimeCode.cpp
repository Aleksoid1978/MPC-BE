/* MIT License
Copyright MediaArea.net SARL
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "TimeCode.h"
#include <limits>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
TimeCode::TimeCode()
{
    memset((char*)this, 0, sizeof(TimeCode));
}

//---------------------------------------------------------------------------
TimeCode::TimeCode(uint32_t Hours, uint8_t Minutes, uint8_t Seconds, uint32_t Frames, uint32_t FramesMax, flags Flags)
    : Frames_(Frames)
    , FramesMax_(FramesMax)
    , Hours_(Hours)
    , Minutes_(Minutes)
    , Seconds_(Seconds)
    , Flags_(Flags.SetValid())
{
    if (GetMinutes() >= 60 || GetSeconds() >= 60 || GetFrames() > GetFramesMax() || (IsDropFrame() && !GetSeconds() && GetFrames() < ((1 + GetFramesMax() / 30) << 1) && (GetMinutes() % 10)))
    {
        *this = TimeCode();
        return;
    }

    if (GetHours() > 24 && IsWrapped24Hours())
        SetHours(GetHours() % 24);
}

//---------------------------------------------------------------------------
TimeCode::TimeCode(uint64_t FrameCount, uint32_t FramesMax, flags Flags)
    : FramesMax_(FramesMax)
    , Flags_(Flags.SetValid())
{
    FromFrames(FrameCount);
}

//---------------------------------------------------------------------------
TimeCode::TimeCode(int64_t FrameCount, uint32_t FramesMax, flags Flags)
    : FramesMax_(FramesMax)
    , Flags_(Flags.SetValid())
{
    FromFrames(FrameCount);
}

//---------------------------------------------------------------------------
TimeCode::TimeCode(double GetSeconds, uint32_t FramesMax, flags Flags, bool Truncate, bool TimeIsDropFrame)
    : FramesMax_(FramesMax)
    , Flags_(Flags.SetValid())
{
    FromSeconds(GetSeconds, Truncate, TimeIsDropFrame);
}

//---------------------------------------------------------------------------
TimeCode::TimeCode(const string_view& Value, uint32_t FramesMax, flags Flags, bool Ignore1001FromDropFrame)
    : FramesMax_(FramesMax)
    , Flags_(Flags.SetValid())
{
    FromString(Value, Ignore1001FromDropFrame);
}

//***************************************************************************
// Operators
//***************************************************************************

//---------------------------------------------------------------------------
static int64_t gcd_compute(int64_t a, int64_t b)
{
    while (b)
    {
        unsigned t = b;
        b = a % b;
        a = t;
    }
    return a;
}

//---------------------------------------------------------------------------
TimeCode& TimeCode::operator +=(const TimeCode& b)
{
    int64_t FrameRate1 = GetFramesMax();
    int64_t FrameRate2 = b.GetFramesMax();
    if (FrameRate1 == FrameRate2)
    {
        *this += b.ToFrames();
        if (b.IsTimed())
            SetTimed();
        return *this;
    }
    FrameRate1++;
    FrameRate2++;
    int64_t Frames1 = ToFrames();
    int64_t Frames2 = b.ToFrames();
    Frames1 *= FrameRate2;
    Frames2 *= FrameRate1;
    auto gcd = gcd_compute(FrameRate1, FrameRate2);
    auto FrameRate = FrameRate1 * FrameRate2 / gcd;
    int64_t Result = (Frames1 + Frames2) / gcd;
    if (Is1001fps() != b.Is1001fps())
    {
        Result *= int64_t(1000 + b.Is1001fps());
        FrameRate *= int64_t(1000 + Is1001fps());
    }
    SetFramesMax(FrameRate - 1);
    FromFrames(Result);
    if (b.IsTimed() && FrameRate1 - 1 != GetFramesMax())
        SetTimed();
    return *this;
}

//---------------------------------------------------------------------------
TimeCode& TimeCode::operator -=(const TimeCode& b)
{
    int64_t FrameRate1 = GetFramesMax();
    int64_t FrameRate2 = b.GetFramesMax();
    if (FrameRate1 == FrameRate2)
    {
        *this -= b.ToFrames();
        if (b.IsTimed())
            SetTimed();
        return *this;
    }
    FrameRate1++;
    FrameRate2++;
    int64_t Frames1 = ToFrames();
    int64_t Frames2 = b.ToFrames();
    Frames1 *= FrameRate2;
    Frames2 *= FrameRate1;
    auto gcd = gcd_compute(FrameRate1, FrameRate2);
    auto FrameRate = FrameRate1 * FrameRate2 / gcd;
    int64_t Result = (Frames1 - Frames2) / gcd;
    if (Is1001fps() != b.Is1001fps())
    {
        Result *= int64_t(1000 + b.Is1001fps());
        FrameRate *= int64_t(1000 + Is1001fps());
    }
    SetFramesMax(FrameRate-1);
    FromFrames(Result);
    if (b.IsTimed() && FrameRate1 - 1 != GetFramesMax())
        SetTimed();
    return *this;
}

//---------------------------------------------------------------------------
void TimeCode::PlusOne()
{
    if (!IsSet())
        return;

    SetFrames(GetFrames() + 1);
    if (GetFrames() > GetFramesMax() || !GetFrames())
    {
        SetSeconds(GetSeconds() + 1);
        SetFrames(0);
        if (GetSeconds() >= 60)
        {
            SetSeconds(0);
            SetMinutes(GetMinutes() + 1);

            if (IsDropFrame() && GetMinutes() % 10)
                SetFrames((1 + (uint64_t)GetFramesMax() / 30) << 1); //frames 0 and 1 (at 30 fps) are dropped for every minutes except 00 10 20 30 40 50

            if (GetMinutes() >= 60)
            {
                SetMinutes(0);
                SetHours(GetHours() + 1);
                if (IsWrapped24Hours() && GetHours() >= 24)
                    SetHours(0);
            }
        }
    }
}

//---------------------------------------------------------------------------
void TimeCode::MinusOne()
{
    if (!IsSet())
        return;

    uint32_t Temp = 0;
    if (IsDropFrame() && GetMinutes() % 10 && !GetSeconds() && GetFrames() == (1 + (uint64_t)GetFramesMax() / 30) << 1)
    {
        Temp = 2 << (int)IsField();
    }
    if (GetFrames() != Temp)
    {
        SetFrames(GetFrames() - 1);
        if (IsNegative() && !GetFrames() && !GetSeconds() && !GetMinutes() && !GetHours())
            SetNegative(false);
        return;
    }

    SetFrames(GetFramesMax());
    if (!GetSeconds())
    {
        SetSeconds(59);
        if (!GetMinutes())
        {
            SetMinutes(59);
            if (!GetHours())
            {
                if (IsWrapped24Hours())
                    SetHours(23);
                else
                {
                    SetNegative();
                    SetHours(0);
                    SetMinutes(0);
                    SetSeconds(0);
                    SetFrames(1);
                }
            }
            else
                SetHours(GetHours() - 1);
        }
        else
            SetMinutes(GetMinutes() - 1);
    }
    else
        SetSeconds(GetSeconds() - 1);
}

//---------------------------------------------------------------------------
static const uint32_t PowersOf10[] =
{
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
};
static const int PowersOf10_Size = sizeof(PowersOf10) / sizeof(int32_t);

//---------------------------------------------------------------------------
int TimeCode::FromString(const string_view& V, bool Ignore1001FromDropFrame)
{
    const char* Value = V.data();
    size_t Length = V.size();

    // Handled undefined
    if (Length >= 8 && !memcmp(Value, "--:--:--", 8))
    {
        bool Caught = false;
        if (Length == 8)
            Caught = true;
        else if (Value[8] == ':' || Value[8] == ';')
        {
            Caught = true;
            for (size_t i = 9; i < Length; i++)
                if (Value[i] != '-')
                    Caught = false;
        }
        if (Caught)
        {
            SetUndefined();
            if (Length > 8 && Value[8] == ';' && Ignore1001FromDropFrame)
                SetDropFrame();
            SetValid();
            return 0;
        }
    }

    // Handle negative values
    if (*Value == '-')
    {
        SetNegative();
        Value++;
        Length--;
    }

    //hh:mm:ss;ff or hh:mm:ss.zzzzzzzzzSfffffffff formats
    if (Length > 7
        && Value[0] >= '0' && Value[0] <= '9'
        && Value[1] >= '0' && Value[1] <= '9'
        && Value[2] == ':'
        && Value[3] >= '0' && Value[3] <= '9'
        && Value[4] >= '0' && Value[4] <= '9'
        && Value[5] == ':'
        && Value[6] >= '0' && Value[6] <= '9'
        && Value[7] >= '0' && Value[7] <= '9')
    {
        if (Length > 8)
        {
            //hh:mm:ss.zzzzzzzzzSfffffffff format
            unsigned char c = (unsigned char)Value[8];
            if (c == '.' || c == ',')
            {
                if (Length == 9)
                {
                    *this = TimeCode();
                    return 1;
                }
                size_t i = 9;
                int32_t S = 0;
                size_t TheoriticalMax = i + PowersOf10_Size;
                size_t MaxLength = Length > TheoriticalMax ? TheoriticalMax : Length;
                while (i < MaxLength)
                {
                    c = (unsigned char)Value[i];
                    c -= '0';
                    if (c > 9)
                        break;
                    S *= 10;
                    S += c;
                    i++;
                }
                if (i == Length)
                    SetFramesMax(PowersOf10[i - 10] - 1);
                else
                {
                    c = (unsigned char)Value[i];
                    if (c != 'S' && c != '/')
                    {
                        *this = TimeCode();
                        return 1;
                    }
                    i++;
                    TheoriticalMax = i + PowersOf10_Size;
                    MaxLength = Length > TheoriticalMax ? TheoriticalMax : Length;
                    uint32_t Multiplier = 0;
                    while (i < Length)
                    {
                        c = (unsigned char)Value[i];
                        c -= '0';
                        if (c > 9)
                            break;
                        Multiplier *= 10;
                        Multiplier += c;
                        i++;
                    }
                    if (i == MaxLength && i < Length && Value[i] == '0' && Multiplier == 100000000)
                    {
                        Multiplier = 1000000000;
                        i++;
                    }
                    if (i < Length)
                    {
                        *this = TimeCode();
                        return 1;
                    }
                    SetFramesMax(Multiplier - 1);
                }
                SetFrames(S);
                SetDropFrame(false);
                Set1001fps(false);
                SetField(false);
                SetTimed();
            }
            //hh:mm:ss;ff format
            else if ((Length == 11 || Length == 12)
                && (Value[8] == ':' || Value[8] == ';')
                && Value[9] >= '0' && Value[9] <= '9'
                && Value[10] >= '0' && Value[10] <= '9'
                && (Length == 11 || (Value[11] >= '0' && Value[11] <= '9')))
            {
                SetFrames(((Value[9] - '0') * 10) + (Value[10] - '0'));
                if (Length == 12)
                    SetFrames(GetFrames() * 10 + (Value[11] - '0'));
                if (Value[8] == ';')
                {
                    SetDropFrame();
                    if (!Ignore1001FromDropFrame)
                        Set1001fps();
                }
                SetTimed(false);
            }
            else
            {
                *this = TimeCode();
                {
                    *this = TimeCode();
                    return 1;
                }
            }
        }
        else
        {
            SetFrames(0);
            SetFramesMax(0);
            SetTimed(false);
        }
        SetHours(((Value[0] - '0') * 10) + (Value[1] - '0'));
        SetMinutes(((Value[3] - '0') * 10) + (Value[4] - '0'));
        SetSeconds(((Value[6] - '0') * 10) + (Value[7] - '0'));
        if (GetMinutes() >= 60 || GetSeconds() >= 60 || GetFrames() > GetFramesMax())
        {
            *this = TimeCode();
            return 1;
        }
        if (IsDropFrame() && GetSeconds() == 0 && (GetFrames() == 0 || GetFrames() == 1) && (GetMinutes() % 10) != 0)
        {
            *this = TimeCode();
            return 1;
        }
        SetValid();
        return 0;
    }

    //Get unit
    if (!Length)
    {
        *this = TimeCode();
        return 1;
    }
    char Unit = Value[Length - 1];
    Length--; //Remove the unit from the string

    switch (Unit)
    {
        //X.X format based on time
    case 's':
    case 'm':
    case 'h':
    {
        if (Unit == 's' && Length && Value[Length - 1] == 'm')
        {
            Length--; //Remove the unit from the string
            Unit = 'n'; //Magic value for ms
        }
        unsigned char c;
        size_t i = 0;
        unsigned S = 0;
        size_t TheoriticalMax = i + PowersOf10_Size;
        size_t MaxLength = Length > TheoriticalMax ? TheoriticalMax : Length;
        while (i < MaxLength)
        {
            c = (unsigned char)Value[i];
            c -= '0';
            if (c > 9)
                break;
            S *= 10;
            S += c;
            i++;
        }
        switch (Unit)
        {
        case 'n':
            SetHours(S / 3600000);
            SetMinutes((S % 3600000) / 60000);
            SetSeconds((S % 60000) / 1000);
            S %= 1000;
            break;
        case 's':
            SetHours(S / 3600);
            SetMinutes((S % 3600) / 60);
            SetSeconds(S % 60);
            break;
        case 'm':
            SetHours(S / 60);
            SetMinutes(S % 60);
            break;
        case 'h':
            SetHours(S);
            break;
        }
        Flags_.reset();
        SetTimed();
        SetValid();
        c = (unsigned char)Value[i];
        if (c == '.' || c == ',')
        {
            i++;
            if (i == Length)
            {
                *this = TimeCode();
                return 1;
            }
            size_t i_Start = i;
            uint64_t T = 0;
            TheoriticalMax = i + PowersOf10_Size;
            MaxLength = Length > TheoriticalMax ? TheoriticalMax : Length;
            while (i < MaxLength)
            {
                c = (unsigned char)Value[i];
                c -= '0';
                if (c > 9)
                    break;
                T *= 10;
                T += c;
                i++;
            }
            if (i != Length)
            {
                *this = TimeCode();
                return 1;
            }
            int FramesRate_Index = i - 1 - i_Start;
            if (FramesRate_Index < 0) {
                *this = TimeCode();
                return 1;
            }
            uint64_t FramesRate = PowersOf10[FramesRate_Index];
            SetFramesMax((uint32_t)FramesRate - 1);
            switch (Unit)
            {
            case 'h':
            {
                T *= 3600;
                uint64_t T_Divider = PowersOf10[2];
                T = (T + T_Divider / 2) / T_Divider;
                FramesRate /= T_Divider;
                uint64_t Temp2 = T / FramesRate;
                SetMinutes(Temp2 / 60);
                if (GetMinutes() >= 60)
                {
                    SetMinutes(0);
                    SetHours(GetHours() + 1);
                }
                SetSeconds(Temp2 % 60);
                T %= FramesRate;
                SetFramesMax((uint32_t)FramesRate - 1);
                break;
            }
            case 'm':
            {
                T *= 60;
                uint64_t T_Divider = PowersOf10[0];
                T = (T + T_Divider / 2) / T_Divider;
                FramesRate /= T_Divider;
                SetSeconds(T / FramesRate);
                if (GetSeconds() >= 60)
                {
                    SetSeconds(0);
                    SetMinutes(GetMinutes() + 1);
                    if (GetMinutes() >= 60)
                    {
                        SetHours(GetHours() + 1);
                        SetMinutes(0);
                    }
                }
                T %= FramesRate;
                SetFramesMax((uint32_t)FramesRate - 1);
                break;
            }
            case 'n':
            {
                FramesRate *= 1000;
                T += ((uint64_t)S) * FramesRate;
                if (FramesRate_Index > 5)
                {
                    uint64_t T_Divider = PowersOf10[8 - FramesRate_Index];
                    T = (T + T_Divider / 2) / T_Divider;
                    FramesRate = PowersOf10[8];
                }
                SetFramesMax((uint32_t)FramesRate - 1);
                break;
            }
            }
            SetFrames(T);
        }
        else if (Unit == 'n')
        {
            SetFrames(S);
            SetFramesMax(999);
        }
        else
        {
            SetFrames(0);
            SetFramesMax(0);
        }
        SetValid();
        return 0;
    }
    break;

    //X format based on rate
    case 'f':
    case 't':
    {
        unsigned char c;
        size_t i = 0;
        unsigned S = 0;
        size_t TheoriticalMax = i + PowersOf10_Size;
        size_t MaxLength = Length > TheoriticalMax ? TheoriticalMax : Length;
        while (i < MaxLength)
        {
            c = (unsigned char)Value[i];
            c -= '0';
            if (c > 9)
                break;
            S *= 10;
            S += c;
            i++;
        }
        if (i != Length)
        {
            *this = TimeCode();
            return 1;
        }
        uint64_t FrameRate = (uint64_t)GetFramesMax() + 1;
        uint32_t OneHourInFrames = 3600 * FrameRate;
        uint32_t OneMinuteInFrames = 60 * FrameRate;
        SetHours(S / OneHourInFrames);
        SetMinutes((S % OneHourInFrames) / OneMinuteInFrames);
        SetSeconds((S % OneMinuteInFrames) / FrameRate);
        SetFrames(S % FrameRate);
        SetTimed(Unit == 't');
        if (IsTimed())
        {
            Set1001fps(false);
            SetDropFrame(false);
            SetField(false);
        }
        SetValid();
        return 0;
    }
    break;
    }

    *this = TimeCode();
    return 1;
}

//---------------------------------------------------------------------------
int TimeCode::FromFrames(uint64_t Frames)
{
    uint64_t Dropped = IsDropFrame() ? (1 + GetFramesMax() / 30) : 0;
    uint64_t FrameRate = (uint64_t)GetFramesMax() + 1;
    uint64_t Dropped2 = Dropped * 2;
    uint64_t Dropped18 = Dropped * 18;

    uint64_t Minutes_Tens = ((uint64_t)Frames) / (600 * FrameRate - Dropped18); //Count of 10 minutes
    uint64_t Minutes_Units = (Frames - Minutes_Tens * (600 * FrameRate - Dropped18)) / (60 * FrameRate - Dropped2);

    Frames += Dropped18 * Minutes_Tens + Dropped2 * Minutes_Units;
    if (Minutes_Units && !((Frames / FrameRate) % 60) && (Frames % FrameRate) < Dropped2) // If Minutes_Tens is not 0 (drop) but count of remaining seconds is 0 and count of remaining frames is less than 2, 1 additional drop was actually counted, removing it
        Frames -= Dropped2;

    int64_t HoursTemp = Frames / (FrameRate * 3600);
    if (HoursTemp >= 24 && IsWrapped24Hours())
        HoursTemp %= 24;
    else if (HoursTemp > numeric_limits<uint32_t>::max())
    {
        *this = TimeCode();
        return 1;
    }
    SetHours(HoursTemp);
    SetNegative(false);
    SetValid();
    auto TotalSeconds = Frames / FrameRate;
    SetMinutes((TotalSeconds / 60) % 60);
    SetSeconds(TotalSeconds % 60);
    SetFrames((uint32_t)(Frames % FrameRate));

    return 0;
}

//---------------------------------------------------------------------------
int TimeCode::FromFrames(int64_t Frames)
{
    auto IsSigned = Frames < 0;
    if (IsSigned)
        Frames = -Frames;
    if (auto Result = FromFrames((uint64_t)Frames))
        return Result;
    SetNegative(IsSigned);
    return 0;
}

//---------------------------------------------------------------------------
int TimeCode::FromSeconds(double Seconds, bool Truncate, bool TimeIsDropFrame)
{
    // Sign
    auto IsSigned = Seconds < 0;
    SetNegative(IsSigned);
    if (IsSigned)
        Seconds = -Seconds;

    // Seconds to frames
    double FrameCountF = Seconds * ((int64_t)GetFramesMax() + 1) / ((!TimeIsDropFrame && Is1001fps()) ? 1.001 : 1) + (Truncate ? 0.0 : 0.5);
    if (FrameCountF > numeric_limits<int64_t>::max() || FrameCountF < numeric_limits<int64_t>::min())
    {
        *this = TimeCode();
        return 1;
    }
    int64_t FrameCountI = (int64_t)FrameCountF;

    // Manage rounding errors, that makes FromSeconds(ToSeconds()) neutral (symetry)
    if (FrameCountF / (FrameCountI + 1) > (double)0.999999999999999)
        FrameCountI++;

    // Compute time code from frames, managing the need to consider time with also drop frames
    if (TimeIsDropFrame && IsDropFrame())
        SetDropFrame(false);
    auto Result = FromFrames(FrameCountI);
    if (TimeIsDropFrame && IsDropFrame())
        SetDropFrame();

    return Result;
}

//---------------------------------------------------------------------------
TimeCode TimeCode::ToRescaled(uint32_t FramesMax, flags Flags, rounding Rounding) const
{
    auto Result = ToFrames();
    auto FrameRate = (uint64_t)GetFramesMax() + 1;
    if (Is1001fps() != Flags.Is1001fps())
    {
        Result *= int64_t(1000 + Is1001fps());
        FrameRate *= int64_t(1000 + Flags.Is1001fps());
    }
    Result *= (uint64_t)FramesMax + 1;
    switch (Rounding)
    {
    case Nearest:
        Result += FrameRate / 2;
        [[fallthrough]];
    case Floor:
        Result /= FrameRate;
        break;
    case Ceil:
    {
        auto NewResult = Result / FrameRate;
        auto NewRemain = Result % FrameRate;
        if (NewRemain)
            NewResult++;
        Result = NewResult;
        break;
    }
    }
    return TimeCode(Result, FramesMax, Flags);
}

//---------------------------------------------------------------------------
string TimeCode::ToString() const
{
    if (!IsValid())
        return {};
    string FrameMaxS = to_string(GetFramesMax());
    auto FrameMax_Length = FrameMaxS.size();
    if (FrameMax_Length < 2)
        FrameMax_Length = 2;
    auto d = IsDropFrame();
    auto t = IsTimed();
    if (!IsSet())
    {
        string TC("--:--:--");
        TC += t ? '.' : (d ? ';' : ':');
        TC.append(FrameMax_Length, '-');
        return TC;
    }
    string TC;
    TC = to_string(GetHours());
    if (TC.size() == 1)
        TC.insert(0, 1, '0');
    if (IsNegative())
        TC.insert(0, 1, '-');
    TC += ':';
    auto MM = GetMinutes();
    TC += ('0' + MM / 10);
    TC += ('0' + MM % 10);
    TC += ':';
    auto SS = GetSeconds();
    TC += ('0' + SS / 10);
    TC += ('0' + SS % 10);
    if (!GetFramesMax())
        return TC;
    TC += t ? '.' : (d ? ';' : ':');
    if (t)
    {
        int AfterCommaMinus1;
        AfterCommaMinus1 = PowersOf10_Size;
        uint64_t FrameRate = (uint64_t)GetFramesMax() + 1;
        while ((--AfterCommaMinus1) >= 0 && PowersOf10[AfterCommaMinus1] != FrameRate);
        if (AfterCommaMinus1 < 0)
        {
            string FramesS = to_string(GetFrames() >> (int)IsField());
            string FrameRateS = to_string(FrameRate);
            if (FramesS.size() < FrameMax_Length)
                TC.append(FrameMax_Length - FramesS.size(), '0');
            TC += FramesS;
            TC += 'S';
            TC += FrameRateS;
        }
        else
        {
            for (int i = 0; i <= AfterCommaMinus1; i++)
                TC += '0' + (GetFrames() / (i == AfterCommaMinus1 ? 1 : PowersOf10[AfterCommaMinus1 - i - 1]) % 10);
        }
    }
    else
    {
        string FramesS = to_string(GetFrames() >> (int)IsField());
        if (FramesS.size() < FrameMax_Length)
            TC.append(FrameMax_Length - FramesS.size(), '0');
        TC += FramesS;
        if (IsField())
        {
            TC += '.';
            TC += ('0' + (GetFrames() & 1));
        }
    }

    return TC;
}

//---------------------------------------------------------------------------
int64_t TimeCode::ToFrames() const
{
    if (!IsSet())
        return 0;

    int64_t TC = (int64_t(GetHours()) * 3600
        + int64_t(GetMinutes()) * 60
        + int64_t(GetSeconds())) * ((int64_t)GetFramesMax() + 1);

    if (IsDropFrame() && GetFramesMax())
    {
        int64_t Dropped = 1 + (int64_t)(GetFramesMax() / 30);

        TC -= int64_t(GetHours()) * 108 * Dropped
            + (int64_t(GetMinutes()) / 10) * 18 * Dropped
            + (int64_t(GetMinutes()) % 10) * 2 * Dropped;
    }
    TC += GetFrames();
    if (IsNegative())
        TC = -TC;

    return TC;
}

//---------------------------------------------------------------------------
int64_t TimeCode::ToMilliseconds() const
{
    if (!IsSet())
        return 0;

    int64_t Den = (uint64_t)GetFramesMax() + 1;
    int64_t MS = (ToFrames() * 1000 * (GetFramesMax() && (IsDropFrame() || Is1001fps()) ? 1.001 : 1.000) + Den / 2) / Den;
    if (IsNegative())
        MS = -MS;

    return MS;
}

//---------------------------------------------------------------------------
double TimeCode::ToSeconds(bool TimeIsDropFrame) const
{
    if (!IsSet())
        return 0;

    const auto FrameRate = (int64_t)GetFramesMax() + 1;
    double Result;
    if (TimeIsDropFrame)
    {
        auto SecondsTotal = GetHours() * 3600
            + GetMinutes() * 60
            + GetSeconds();
        const auto Frames = SecondsTotal * FrameRate + GetFrames();
        Result = ((double)Frames) / FrameRate;
    }
    else
    {
        Result = ((double)ToFrames()) / FrameRate;
        if (Is1001fps())
            Result *= 1.001;
    }
    if (IsNegative())
        Result = -Result;

    return Result;
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
//Modified Julian Date
std::string Date_MJD(uint16_t Date_)
{
    //Calculating
    double Date = Date_;
    int Y2 = (int)((Date - 15078.2) / 365.25);
    int M2 = (int)(((Date - 14956.1) - ((int)(Y2 * 365.25))) / 30.6001);
    int D = (int)(Date - 14956 - ((int)(Y2 * 365.25)) - ((int)(M2 * 30.6001)));
    int K = 0;
    if (M2 == 14 || M2 == 15)
        K = 1;
    int Y = Y2 + K;
    int M = M2 - 1 - K * 12;

    //Formatting
    return               to_string(1900 + Y) + '-'
        + (M < 10 ? "0" : "") + to_string(M) + '-'
        + (D < 10 ? "0" : "") + to_string(D);
}

//---------------------------------------------------------------------------
//Form: HHMMSS, BCD
std::string Time_BCD(uint32_t Time)
{
    string V("00:00:00");
    V[0] += (uint8_t)(Time >> 20) & 0x0F;
    V[1] += (uint8_t)(Time >> 16) & 0x0F;
    V[3] += (uint8_t)(Time >> 12) & 0x0F;
    V[4] += (uint8_t)(Time >> 8) & 0x0F;
    V[6] += (uint8_t)(Time >> 4) & 0x0F;
    V[7] += (uint8_t)(Time) & 0x0F;
    return V;
}

} //NameSpace

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
#include "MediaInfo/TimeCode.h"
#include <limits>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
TimeCode::TimeCode ()
{
    memset(this, 0, sizeof(TimeCode));
}

//---------------------------------------------------------------------------
TimeCode::TimeCode (int32u Hours_, int8u Minutes_, int8u Seconds_, int32u Frames_, int32u FramesMax_, bool DropFrame_, bool MustUseSecondField_, bool IsSecondField_)
:   Hours(Hours_),
    Minutes(Minutes_),
    Seconds(Seconds_),
    Frames(Frames_),
    FramesMax(FramesMax_)
{
    if (DropFrame_)
        Flags.set(DropFrame);
    if (MustUseSecondField_)
        Flags.set(MustUseSecondField);
    if (IsSecondField_)
        Flags.set(IsSecondField);
    Flags.set(IsValid);
}

//---------------------------------------------------------------------------
TimeCode::TimeCode (int64s Frames_, int32u FramesMax_, bool DropFrame_, bool MustUseSecondField_, bool IsSecondField_)
:   FramesMax(FramesMax_)
{
    if (DropFrame_)
        Flags.set(DropFrame);
    if (MustUseSecondField_)
        Flags.set(MustUseSecondField);
    if (IsSecondField_)
        Flags.set(IsSecondField);
    FromFrames(Frames_);
}

bool TimeCode::FromFrames(int64s Frames_)
{
    if (Frames_<0)
    {
        Flags.set(IsNegative);
        Frames_=-Frames_;
    }
    else
        Flags.reset(IsNegative);

    int64u Dropped=Flags.test(DropFrame)?(1+FramesMax/30):0;
    int32u FrameRate=(int32u)FramesMax+1;
    int64u Dropped2=Dropped*2;
    int64u Dropped18=Dropped*18;

    int64u Minutes_Tens = ((int64u)Frames_)/(600*FrameRate-Dropped18); //Count of 10 minutes
    int64u Minutes_Units = (Frames_-Minutes_Tens*(600*FrameRate-Dropped18))/(60*FrameRate-Dropped2);

    Frames_+=Dropped18*Minutes_Tens+Dropped2*Minutes_Units;
    if (Minutes_Units && ((Frames_/FrameRate)%60)==0 && (Frames_%FrameRate)<Dropped2) // If Minutes_Tens is not 0 (drop) but count of remaining seconds is 0 and count of remaining frames is less than 2, 1 additional drop was actually counted, removing it
        Frames_-=Dropped2;

    int64s HoursTemp=(((Frames_/FrameRate)/60)/60);
    if (HoursTemp>(int32u)-1)
    {
        Hours=(int32u)-1;
        Minutes=59;
        Seconds=59;
        Frames=FramesMax;
        return true;
    }
    Flags.reset(IsTime);
    Flags.set(IsValid);
    Hours=(int8u)HoursTemp;
    auto TotalSeconds=Frames_/FrameRate;
    Minutes=(TotalSeconds/60)%60;
    Seconds=TotalSeconds%60;
    Frames=(int32u)(Frames_%FrameRate);

    return false;
}

//---------------------------------------------------------------------------
TimeCode::TimeCode (const char* Value, size_t Length)
:   FramesMax(0)
{
    FromString(Value, Length);
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void TimeCode::PlusOne()
{
    //TODO: negative values

    if (Flags.test(HasNoFramesInfo))
        return;

    if (Flags.test(MustUseSecondField))
    {
        if (Flags.test(IsSecondField))
        {
            Frames++;
            Flags.reset(IsSecondField);
        }
        else
            Flags.set(IsSecondField);
    }
    else
        Frames++;
    if (Frames>FramesMax || !Frames)
    {
        Seconds++;
        Frames=0;
        if (Seconds>=60)
        {
            Seconds=0;
            Minutes++;

            if (Flags.test(DropFrame) && Minutes%10)
                Frames=(1+FramesMax/30)*2; //frames 0 and 1 (at 30 fps) are dropped for every minutes except 00 10 20 30 40 50

            if (Minutes>=60)
            {
                Minutes=0;
                Hours++;
                if (Hours>=24)
                {
                    Hours=0;
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
void TimeCode::MinusOne()
{
    //TODO: negative values

    if (Flags.test(HasNoFramesInfo))
        return;

    if (Flags.test(MustUseSecondField) && Flags.test(IsSecondField))
    {
        Flags.reset(IsSecondField);
        return;
    }

    bool d=Flags.test(DropFrame);
    if (!FramesMax && (!Frames || d))
        return;
    if (Flags.test(MustUseSecondField))
        Flags.set(IsSecondField);

    if (Frames && !(d && Minutes%10 && Frames<(1+FramesMax/30)*2))
    {
        Frames--;
        return;
    }

    Frames=FramesMax;
    if (!Seconds)
    {
        Seconds=59;
        if (!Minutes)
        {
            Minutes=59;
            if (!Hours)
                Hours=24;
            else
                Hours--;
        }
        else
            Minutes--;
    }
    else
        Seconds--;
}

//---------------------------------------------------------------------------
static const int32s PowersOf10[]=
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
static const int PowersOf10_Size=sizeof(PowersOf10)/sizeof(int32s);

//---------------------------------------------------------------------------
bool TimeCode::FromString(const char* Value, size_t Length)
{
    //hh:mm:ss;ff or hh:mm:ss.zzzzzzzzzSfffffffff formats
    if (Length>7
     && Value[0]>='0' && Value[0]<='9'
     && Value[1]>='0' && Value[1]<='9'
     && Value[2]==':'
     && Value[3]>='0' && Value[3]<='9'
     && Value[4]>='0' && Value[4]<='9'
     && Value[5]==':'
     && Value[6]>='0' && Value[6]<='9'
     && Value[7]>='0' && Value[7]<='9')
    {
        if (Length>8)
        {
            //hh:mm:ss.zzzzzzzzzSfffffffff format
            unsigned char c=(unsigned char)Value[8];
            if (c=='.' || c==',')
            {
                if (Length==9)
                    return true;
                int i=9;
                int32s S=0;
                int TheoriticalMax=i+PowersOf10_Size;
                int MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
                while (i<MaxLength)
                {
                    c=(unsigned char)Value[i];
                    c-='0';
                    if (c>9)
                        break;
                    S*=10;
                    S+=c;
                    i++;
                }
                if (i==Length)
                    FramesMax=PowersOf10[i-10]-1;
                else
                {
                    c=(unsigned char)Value[i];
                    if (c!='S' && c!='/')
                        return true;
                    i++;
                    TheoriticalMax=i+PowersOf10_Size;
                    MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
                    int32u Multiplier=0;
                    while (i<Length)
                    {
                        c=(unsigned char)Value[i];
                        c-='0';
                        if (c>9)
                            break;
                        Multiplier*=10;
                        Multiplier+=c;
                    }
                    if (i==MaxLength && i<Length && Value[i]=='0' && Multiplier==100000000)
                    {
                        Multiplier=1000000000;
                        i++;
                    }
                    if (i<Length)
                        return true;
                    FramesMax=Multiplier-1;
                }
                Frames=S;
                Flags.reset(DropFrame);
                Flags.reset(FramesPerSecond_Is1001);
                Flags.reset(MustUseSecondField);
                Flags.reset(IsSecondField);
                Flags.reset(IsNegative);
                Flags.reset(HasNoFramesInfo);
                Flags.set(IsTime);
            }
            //hh:mm:ss;ff format
            else if (Length==11
             && (Value[8]==':' || Value[8]==';')
             && Value[9]>='0' && Value[9]<='9'
             && Value[10]>='0' && Value[10]<='9')
            {
                Frames=((Value[9]-'0')*10)+(Value[10]-'0');
                Flags.set(DropFrame, Value[8]==';');
                if (Value[8])
                    Flags.set(FramesPerSecond_Is1001);
                Flags.reset(IsSecondField);
                Flags.reset(IsNegative);
                Flags.reset(HasNoFramesInfo);
                Flags.reset(IsTime);
            }
            else
            {
                *this=TimeCode();
                return true;
            }
        }
        else
        {
            Frames=0;
            FramesMax=0;
            Flags.reset(IsSecondField);
            Flags.reset(IsNegative);
            Flags.set(HasNoFramesInfo);
            Flags.reset(IsTime);
        }
        Hours=((Value[0]-'0')*10)+(Value[1]-'0');
        Minutes=((Value[3]-'0')*10)+(Value[4]-'0');
        Seconds=((Value[6]-'0')*10)+(Value[7]-'0');
        Flags.set(IsValid);
        return false;
    }

    //Get unit
    if (!Length)
    {
        *this=TimeCode();
        return true;
    }
    char Unit=Value[Length-1];
    Length--; //Remove the unit from the string

    switch (Unit)
    {
        //X.X format based on time
        case 's':
        case 'm':
        case 'h':
            {
            if (Unit=='s' && Length && Value[Length-1]=='m')
            {
                Length--; //Remove the unit from the string
                Unit='n'; //Magic value for ms
            }
            unsigned char c;
            int i=0;
            int32s S=0;
            int TheoriticalMax=i+PowersOf10_Size;
            int MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
            while (i<MaxLength)
            {
                c=(unsigned char)Value[i];
                c-='0';
                if (c>9)
                    break;
                S*=10;
                S+=c;
                i++;
            }
            switch (Unit)
            {
                case 'n':
                    Hours=S/3600000;
                    Minutes=(S%3600000)/60000;
                    Seconds=(S%60000)/1000;
                    S%=1000;
                    break;
                case 's':
                    Hours=S/3600;
                    Minutes=(S%3600)/60;
                    Seconds=S%60;
                    break;
                case 'm':
                    Hours=S/60;
                    Minutes=S%60;
                    break;
                case 'h':
                    Hours=S;
                    break;
            }
            Flags.reset();
            Flags.set(IsTime);
            Flags.set(IsValid);
            c=(unsigned char)Value[i];
            if (c=='.' || c==',')
            {
                i++;
                if (i==Length)
                    return true;
                size_t i_Start=i;
                int64u T=0;
                TheoriticalMax=i+PowersOf10_Size;
                MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
                while (i<MaxLength)
                {
                    c=(unsigned char)Value[i];
                    c-='0';
                    if (c>9)
                        break;
                    T*=10;
                    T+=c;
                    i++;
                }
                if (i!=Length)
                    return true;
                int FramesRate_Index=i-1-i_Start;
                int64u FramesRate=PowersOf10[FramesRate_Index];
                FramesMax=(int32u)(FramesRate-1);
                switch (Unit)
                {
                    case 'h':
                    {
                        T*=3600;
                        int64u T_Divider=PowersOf10[2];
                        T=(T+T_Divider/2)/T_Divider;
                        FramesRate/=T_Divider;
                        int64u Temp2=T/FramesRate;
                        Minutes=Temp2/60;
                        if (Minutes>=60)
                        {
                            Minutes=0;
                            Hours++;
                        }
                        Seconds=Temp2%60;
                        T%=FramesRate;
                        FramesMax=FramesRate-1;
                        break;
                    }
                    case 'm':
                    {
                        T*=60;
                        int64u T_Divider=PowersOf10[0];
                        T=(T+T_Divider/2)/T_Divider;
                        FramesRate/=T_Divider;
                        Seconds=T/FramesRate;
                        if (Seconds>=60)
                        {
                            Seconds=0;
                            Minutes++;
                            if (Minutes>=60)
                            {
                                Hours++;
                                Minutes=0;
                            }
                        }
                        T%=FramesRate;
                        FramesMax=FramesRate-1;
                        break;
                    }
                    case 'n':
                    {
                        FramesRate*=1000;
                        T+=((int64u)S)*FramesRate;
                        int64u T_Divider=PowersOf10[1];
                        if (FramesRate_Index>5)
                        {
                            int64u T_Divider=PowersOf10[8-FramesRate_Index];
                            T=(T+T_Divider/2)/T_Divider;
                            FramesRate=PowersOf10[8];
                        }
                        FramesMax=(int32u)(FramesRate-1);
                        break;
                    }
                }
                Frames=T;
            }
            else if (Unit=='n')
            {
                Frames=S;
                FramesMax=1000;
            }
            else
            {
                Frames=0;
                FramesMax=0;
            }
            Flags.set(IsValid);
            return false;
            }
            break;

        //X format based on rate
        case 'f':
        case 't':
            {
            unsigned char c;
            int i=0;
            int32s S=0;
            int TheoriticalMax=i+PowersOf10_Size;
            int MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
            while (i<MaxLength)
            {
                c=(unsigned char)Value[i];
                c-='0';
                if (c>9)
                    break;
                S*=10;
                S+=c;
                i++;
            }
            if (i!=Length)
                return true;
            int32u FrameRate=(int32u)FramesMax+1;
            int32s OneHourInFrames=3600*FrameRate;
            int32s OneMinuteInFrames=60*FrameRate;
            Hours=S/OneHourInFrames;
            Minutes=(S%OneHourInFrames)/OneMinuteInFrames;
            Seconds=(S%OneMinuteInFrames)/FrameRate;
            Frames=S%FrameRate;
            Flags.reset(MustUseSecondField);
            Flags.reset(IsSecondField);
            Flags.reset(IsNegative);
            Flags.reset(HasNoFramesInfo);
            Flags.set(IsTime, Unit=='t');
            Flags.set(IsValid);
            return false;
            }
            break;
    }

    *this=TimeCode();
    return true;
}

//---------------------------------------------------------------------------
string TimeCode::ToString() const
{
    if (!HasValue())
        return string();
    string TC;
    if (Flags.test(IsNegative))
        TC+='-';
    int8u HH=Hours;
    if (HH>100)
    {
        TC+=to_string(HH/100);
        HH%=100;
    }
    TC+=('0'+HH/10);
    TC+=('0'+HH%10);
    TC+=':';
    int8u MM=Minutes;
    if (MM>100)
    {
        TC+=('0'+MM/100);
        MM%=100;
    }
    TC+=('0'+MM/10);
    TC+=('0'+MM%10);
    TC+=':';
    int8u SS=Seconds;
    if (SS>100)
    {
        TC+=('0'+SS/100);
        SS%=100;
    }
    TC+=('0'+SS/10);
    TC+=('0'+SS%10);
    bool d=Flags.test(DropFrame);
    bool t=Flags.test(IsTime);
    if (!t && d)
        TC+=';';
    if (t)
    {
        int AfterCommaMinus1;
        AfterCommaMinus1=PowersOf10_Size;
        auto FrameRate=FramesMax+1;
        while ((--AfterCommaMinus1)>=0 && PowersOf10[AfterCommaMinus1]!=FrameRate);
        TC+='.';
        if (AfterCommaMinus1<0)
        {
            stringstream s;
            s<<Frames;
            TC+=s.str();
            TC+='S';
            s.str(string());
            s<<FrameRate;
            TC+=s.str();
        }
        else
        {
            for (int i=0; i<=AfterCommaMinus1;i++)
                TC+='0'+(Frames/(i==AfterCommaMinus1?1:PowersOf10[AfterCommaMinus1-i-1])%10);
        }
    }
    else if (!Flags.test(HasNoFramesInfo))
    {
        if (!d)
            TC+=':';
        auto FF=Frames;
        if (FF>=100)
        {
            TC+=to_string(FF/100);
            FF%=100;
        }
        TC+=('0'+(FF/10));
        TC+=('0'+(FF%10));
        if (Flags.test(MustUseSecondField) || Flags.test(IsSecondField))
        {
            TC+='.';
            TC+=('0'+Flags.test(IsSecondField));
        }
    }

    return TC;
}

//---------------------------------------------------------------------------
int64s TimeCode::ToFrames() const
{
    if (!HasValue())
        return 0;

    int64s TC=(int64s(Hours)     *3600
             + int64s(Minutes)   *  60
             + int64s(Seconds)        )*(FramesMax+1);

    if (Flags.test(DropFrame) && FramesMax)
    {
        int64u Dropped=FramesMax/30+1;

        TC-= int64s(Hours)      *108*Dropped
          + (int64s(Minutes)/10)*18*Dropped
          + (int64s(Minutes)%10)* 2*Dropped;
    }

    if (!Flags.test(HasNoFramesInfo) && FramesMax)
        TC+=Frames;
    if (Flags.test(MustUseSecondField))
        TC<<=1;
    if (Flags.test(IsSecondField))
        TC++;
    if (Flags.test(IsNegative))
        TC=-TC;

    return TC;
}

//---------------------------------------------------------------------------
int64s TimeCode::ToMilliseconds() const
{
    if (!HasValue())
        return 0;

    int64s MS=float64_int64s(ToFrames()*1000*(FramesMax && (Flags.test(DropFrame) || Flags.test(FramesPerSecond_Is1001))?1.001:1.000)/((((int64u)FramesMax)+1)*(Flags.test(MustUseSecondField)?2:1)));

    if (Flags.test(IsNegative))
        MS=-MS;

    return MS;
}

//***************************************************************************
//
//***************************************************************************

//---------------------------------------------------------------------------
//Modified Julian Date
Ztring Date_MJD(int16u Date_)
{
    //Calculating
    float64 Date=Date_;
    int Y2=(int)((Date-15078.2)/365.25);
    int M2=(int)(((Date-14956.1) - ((int)(Y2*365.25))) /30.6001);
    int D =(int)(Date-14956 - ((int)(Y2*365.25)) - ((int)(M2*30.6001)));
    int K=0;
    if (M2==14 || M2==15)
        K=1;
    int Y =Y2+K;
    int M =M2-1-K*12;

    //Formatting
    return                       Ztring::ToZtring(1900+Y)+__T("-")
         + (M<10?__T("0"):__T(""))+Ztring::ToZtring(     M)+__T("-")
         + (D<10?__T("0"):__T(""))+Ztring::ToZtring(     D);
}

//---------------------------------------------------------------------------
//Form: HHMMSS, BCD
Ztring Time_BCD(int32u Time)
{
    return (((Time>>16)&0xFF)<10?__T("0"):__T("")) + Ztring::ToZtring((Time>>16)&0xFF, 16)+__T(":") //BCD
         + (((Time>> 8)&0xFF)<10?__T("0"):__T("")) + Ztring::ToZtring((Time>> 8)&0xFF, 16)+__T(":") //BCD
         + (((Time    )&0xFF)<10?__T("0"):__T("")) + Ztring::ToZtring((Time    )&0xFF, 16);        //BCD
}


} //NameSpace

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
:   Hours((int8u)-1),
    Minutes((int8u)-1),
    Seconds((int8u)-1),
    Frames((int8u)-1),
    MoreSamples(0),
    MoreSamples_Frequency(0),
    FramesPerSecond_Is1001(false),
    FramesPerSecond(0),
    DropFrame(false),
    MustUseSecondField(false),
    IsSecondField(false),
    IsNegative(false)
{
}

//---------------------------------------------------------------------------
TimeCode::TimeCode (int8u Hours_, int8u Minutes_, int8u Seconds_, int8u Frames_, int8u FramesPerSecond_, bool DropFrame_, bool MustUseSecondField_, bool IsSecondField_)
:   Hours(Hours_),
    Minutes(Minutes_),
    Seconds(Seconds_),
    Frames(Frames_),
    MoreSamples(0),
    MoreSamples_Frequency(0),
    FramesPerSecond_Is1001(false),
    FramesPerSecond(FramesPerSecond_),
    DropFrame(DropFrame_),
    MustUseSecondField(MustUseSecondField_),
    IsSecondField(IsSecondField_),
    IsNegative(false)
{
}

//---------------------------------------------------------------------------
TimeCode::TimeCode (int64s Frames_, int8u FramesPerSecond_, bool DropFrame_, bool MustUseSecondField_, bool IsSecondField_)
:   FramesPerSecond(FramesPerSecond_),
    MoreSamples(0),
    MoreSamples_Frequency(0),
    FramesPerSecond_Is1001(false),
    DropFrame(DropFrame_),
    MustUseSecondField(MustUseSecondField_),
    IsSecondField(IsSecondField_)
{
    if (!FramesPerSecond_)
    {
        Frames  = 0;
        Seconds = 0;
        Minutes = 0;
        Hours   = 0;
        IsNegative = true; //Forcing a weird display
        return;
    }

    if (Frames_<0)
    {
        IsNegative=true;
        Frames_=-Frames_;
    }
    else
        IsNegative=false;

    int8u Dropped=0;
    if (DropFrame_)
    {
        Dropped=2;
        if (FramesPerSecond_>30)
            Dropped+=2;
        if (FramesPerSecond_>60)
            Dropped+=2;
        if (FramesPerSecond_>90)
            Dropped+=2;
        if (FramesPerSecond_>120)
            Dropped+=2;
    }

    int64u Minutes_Tens = Frames_/(600*FramesPerSecond_-Dropped*9); //Count of 10 minutes
    int64u Minutes_Units = (Frames_-Minutes_Tens*(600*FramesPerSecond_-Dropped*9))/(60*FramesPerSecond_-Dropped);

    Frames_ += 9*Dropped*Minutes_Tens+Dropped*Minutes_Units;
    if (Minutes_Units && ((Frames_/FramesPerSecond_)%60)==0 && (Frames_%FramesPerSecond_)<Dropped) // If Minutes_Tens is not 0 (drop) but count of remaining seconds is 0 and count of remaining frames is less than 2, 1 additional drop was actually counted, removing it
        Frames_-=Dropped;

    Frames  =    Frames_ % FramesPerSecond_;
    Seconds =   (Frames_ / FramesPerSecond_) % 60;
    Minutes =  ((Frames_ / FramesPerSecond_) / 60) % 60;
    int64s Temp = (((Frames_ / FramesPerSecond_) / 60) / 60);
    Hours = (Temp>99 || Temp<-99)?(Temp%24):Temp;
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void TimeCode::PlusOne()
{
    //TODO: negative values

    if (FramesPerSecond==0)
        return;
    if (MustUseSecondField)
    {
        if (IsSecondField)
        {
            Frames++;
            IsSecondField=false;
        }
        else
            IsSecondField=true;
    }
    else
        Frames++;
    if (Frames>=FramesPerSecond)
    {
        Seconds++;
        Frames=0;
        if (Seconds>=60)
        {
            Seconds=0;
            Minutes++;

            if (DropFrame && Minutes%10)
                Frames=2; //frames 0 and 1 are dropped for every minutes except 00 10 20 30 40 50

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

    if (FramesPerSecond==0)
        return;
    if (MustUseSecondField && IsSecondField)
        IsSecondField=false;
    else
    {
        if (Frames==0 || (DropFrame && Minutes%10 && Frames<=2))
        {
            Frames=FramesPerSecond;
            if (Seconds==0)
            {
                Seconds=60;
                if (Minutes==0)
                {
                    Minutes=60;
                    if (Hours==0)
                        Hours=24;
                    Hours--;
                }
                Minutes--;
            }
            Seconds--;
        }
        Frames--;

        if (MustUseSecondField)
            IsSecondField=true;
    }
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
                    MoreSamples_Frequency=PowersOf10[i-10];
                else
                {
                    c=(unsigned char)Value[i];
                    if (c!='S' && c!='/')
                        return true;
                    i++;
                    TheoriticalMax=i+PowersOf10_Size;
                    MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
                    int32s Multiplier=0;
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
                    MoreSamples_Frequency=Multiplier;
                }
                MoreSamples=S;
                DropFrame=false;
            }
            //hh:mm:ss;ff format
            else if (Length==11
             && (Value[8]==':' || Value[8]==';')
             && Value[9]>='0' && Value[9]<='9'
             && Value[10]>='0' && Value[10]<='9')
            {
                DropFrame=Value[8]==';';
                Frames=((Value[9]-'0')*10)+(Value[10]-'0');
            }
            else
                return true;
        }
        Hours=((Value[0]-'0')*10)+(Value[1]-'0');
        Minutes=((Value[3]-'0')*10)+(Value[4]-'0');
        Seconds=((Value[6]-'0')*10)+(Value[7]-'0');
        return false;
    }
    //X.Xs format
    if (Length>=2
     && Value[Length-1]=='s')
    {
        Length--; //Remove the "s" from the string
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
        Hours=S/3600;
        Minutes=(S%3600)/60;
        Seconds=S%60;
        c=(unsigned char)Value[i];
        if (c=='.' || c==',')
        {
            i++;
            if (i==Length)
                return true;
            S=0;
            TheoriticalMax=i+PowersOf10_Size;
            MaxLength=Length>TheoriticalMax?TheoriticalMax:Length;
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
            MoreSamples_Frequency=PowersOf10[i-10];
            MoreSamples=S;
        }
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
string TimeCode::ToString()
{
    if (!FramesPerSecond)
        return string();

    string TC;
    if (IsNegative)
        TC+='-';
    TC+=('0'+Hours/10);
    TC+=('0'+Hours%10);
    TC+=':';
    TC+=('0'+Minutes/10);
    TC+=('0'+Minutes%10);
    TC+=':';
    TC+=('0'+Seconds/10);
    TC+=('0'+Seconds%10);
    if (MoreSamples && MoreSamples_Frequency)
    {
        int AfterCommaMinus1=0;
        if (MoreSamples>0)
        {
            AfterCommaMinus1=PowersOf10_Size;
            while ((--AfterCommaMinus1)>=0 && PowersOf10[AfterCommaMinus1]!=MoreSamples_Frequency);
            TC+=AfterCommaMinus1>=0?'.':'+';
        }
        else
        {
            AfterCommaMinus1=-1;
            TC+='-';
            MoreSamples=-MoreSamples;
        }
        stringstream s;
        s<<MoreSamples;
        TC+=s.str();
        TC+='S';
        s.str(string());
        s<<MoreSamples_Frequency;
        TC+=s.str();
    }
    else
    {
        TC+=DropFrame?';':':';
        TC+=('0'+(Frames*(MustUseSecondField?2:1)+(IsSecondField?1:0))/10);
        TC+=('0'+(Frames*(MustUseSecondField?2:1)+(IsSecondField?1:0))%10);
    }

    return TC;
}

//---------------------------------------------------------------------------
int64s TimeCode::ToFrames()
{
    if (!FramesPerSecond)
        return 0;

    int64s TC=(int64s(Hours)     *3600
             + int64s(Minutes)   *  60
             + int64s(Seconds)        )*int64s(FramesPerSecond)
             + int64s(Frames);

    if (DropFrame)
    {
        TC-= int64s(Hours)      *108
          + (int64s(Minutes)/10)*18
          + (int64s(Minutes)%10)*2;
    }

    TC*=(MustUseSecondField?2:1);
    TC+=(IsSecondField?1:0);

    return IsNegative?-TC:TC;
}

//---------------------------------------------------------------------------
int64s TimeCode::ToMilliseconds()
{
    if (!FramesPerSecond)
    {
        if (!MoreSamples_Frequency)
            return 0;
        int64s TC=(int64s(Hours)     *3600
                 + int64s(Minutes)   *  60
                 + int64s(Seconds)        )*1000;
        TC+=((int64s)MoreSamples)*1000/MoreSamples_Frequency;
        return TC;
    }

    int64s MS=float64_int64s(ToFrames()*1000*(FramesPerSecond_Is1001?1.001:1.000)/(FramesPerSecond*(MustUseSecondField?2:1)));

    return IsNegative?-MS:MS;
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

    //Formating
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

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
#if defined(MEDIAINFO_MPEG7_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_Mpeg7.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include "MediaInfo/OutputHelpers.h"
#include <ctime>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
//---------------------------------------------------------------------------

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const Char* Mpeg7_Type(MediaInfo_Internal &MI) //TO ADAPT
{
    if (MI.Count_Get(Stream_Image))
    {
        if (MI.Count_Get(Stream_Video) || MI.Count_Get(Stream_Audio))
            return __T("Multimedia");
        else
            return __T("Image");
    }
    else if (MI.Count_Get(Stream_Video))
    {
        if (MI.Count_Get(Stream_Audio))
            return __T("AudioVisual");
        else
            return __T("Video");
    }
    else if (MI.Count_Get(Stream_Audio))
        return __T("Audio");

    //Not known
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);
    if (Format==__T("AVI") || Format==__T("DV") || Format==__T("MPEG-4") || Format==__T("MPEG-PS") || Format==__T("MPEG-TS") || Format==__T("QuickTime") || Format==__T("Windows Media"))
        return __T("Video");
    if (Format==__T("MPEG Audio") || Format==__T("Wave"))
        return __T("Audio");
    if (Format==__T("BMP") || Format==__T("GIF") || Format==__T("JPEG") || Format==__T("JPEG 2000") || Format==__T("PNG") || Format==__T("TIFF"))
        return __T("Image");
    return __T("Multimedia");
}

//---------------------------------------------------------------------------
int32u Mpeg7_ContentCS_termID(MediaInfo_Internal &MI)
{
    if (MI.Count_Get(Stream_Image))
    {
        if (MI.Count_Get(Stream_Video) || MI.Count_Get(Stream_Audio))
            return 20000;
        else
            return 40100;
    }
    else if (MI.Count_Get(Stream_Video))
    {
        if (MI.Count_Get(Stream_Audio))
            return 20000;
        else
            return 40200;
    }
    else if (MI.Count_Get(Stream_Audio))
        return 10000;

    //Not known
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);
    if (Format==__T("AVI") || Format==__T("DV") || Format==__T("MPEG-4") || Format==__T("MPEG-PS") || Format==__T("MPEG-TS") || Format==__T("QuickTime") || Format==__T("Windows Media"))
        return 40200;
    if (Format==__T("MPEG Audio") || Format==__T("Wave"))
        return 10000;
    if (Format==__T("BMP") || Format==__T("GIF") || Format==__T("JPEG") || Format==__T("JPEG 2000") || Format==__T("PNG") || Format==__T("TIFF"))
        return 40100;
    return 500000;
}

Ztring Mpeg7_ContentCS_Name(int32u termID, MediaInfo_Internal &) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case  1 : return __T("Audio");
        case  2 : return __T("Audiovisual");
        case  3 : return __T("Scene");
        case  4 :   switch ((termID%10000)/100)
                    {
                        case 1 : return __T("Image");
                        case 2 : return __T("Video");
                        case 3 : return __T("Graphics");
                    }
        case 50 : return Ztring(); //Unknown
        default : return Ztring();
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_FileFormatCS_termID_MediaInfo(MediaInfo_Internal &MI)
{
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);

    if (Format==__T("MPEG Audio"))
    {
        if (MI.Get(Stream_Audio, 0, Audio_Format_Profile).find(__T('2'))!=string::npos)
            return 500000; //mp2
        if (MI.Get(Stream_Audio, 0, Audio_Format_Profile).find(__T('1'))!=string::npos)
            return 510000; //mp1
        return 0;
    }
    if (Format==__T("Wave") && MI.Get(Stream_General, 0, General_Format_Profile)==__T("RF64"))
        return 520000; //Wav (RF64)
    if (Format==__T("Wave64"))
        return 530000;
    return 0;
}

//---------------------------------------------------------------------------
int32u Mpeg7_FileFormatCS_termID(MediaInfo_Internal &MI)
{
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);

    if (Format==__T("AVI"))
        return 70000;
    if (Format==__T("BMP"))
        return 110000;
    if (Format==__T("GIF"))
        return 120000;
    if (Format==__T("DV"))
        return 60000;
    if (Format==__T("JPEG"))
        return 10000;
    if (Format==__T("JPEG 2000"))
        return 20000;
    if (Format==__T("MPEG Audio"))
        return (MI.Get(Stream_Audio, 0, Audio_Format_Profile).find(__T('3'))!=string::npos)?40000:0;
    if (Format==__T("MPEG-4"))
        return 50000;
    if (Format==__T("MPEG-PS"))
        return 30100;
    if (Format==__T("MPEG-TS"))
        return 30200;
    if (Format==__T("PNG"))
        return 150000;
    if (Format==__T("QuickTime"))
        return 160000;
    if (Format==__T("TIFF"))
        return 180000;
    if (Format==__T("Wave"))
    {
        if (!MI.Get(Stream_General, 0, General_Format_Profile).empty())
            return Mpeg7_FileFormatCS_termID_MediaInfo(MI); //Out of specs
        else
            return 90000;
    }
    if (Format==__T("Windows Media"))
        return 190000;
    if (Format==__T("ZIP"))
        return 100000;

    //Out of specs
    return Mpeg7_FileFormatCS_termID_MediaInfo(MI);
}

Ztring Mpeg7_FileFormatCS_Name(int32u termID, MediaInfo_Internal &MI) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case  1 : return __T("jpeg");
        case  2 : return __T("JPEG 2000");
        case  3 :   switch ((termID%10000)/100)
                    {
                        case 1 : return __T("mpeg-ps");
                        case 2 : return __T("mpeg-ts");
                        default: return __T("mpeg");
                    }
        case  4 : return __T("mp3");
        case  5 : return __T("mp4");
        case  6 : return __T("dv");
        case  7 : return __T("avi");
        case  8 : return __T("bdf");
        case  9 : return __T("wav");
        case 10 : return __T("zip");
        case 11 : return __T("bmp");
        case 12 : return __T("gif");
        case 13 : return __T("photocd");
        case 14 : return __T("ppm");
        case 15 : return __T("png");
        case 16 : return __T("quicktime");
        case 17 : return __T("spiff");
        case 18 : return __T("tiff");
        case 19 : return __T("asf");
        case 20 : return __T("iff");
        case 21 : return __T("miff");
        case 22 : return __T("pcx");
        //Out of specs --> MediaInfo CS
        case 50 : return __T("mp1");
        case 51 : return __T("mp2");
        case 52 : return __T("wav-rf64");
        case 53 : return __T("wave64");
        default : return MI.Get(Stream_General, 0, General_Format);
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_VisualCodingFormatCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Video, StreamPos, Video_Format);
    const Ztring &Version=MI.Get(Stream_Video, StreamPos, Video_Format_Version);
    const Ztring &Profile=MI.Get(Stream_Video, StreamPos, Video_Format_Profile);

    if (Format==__T("MPEG Video"))
    {
        if (Version.find(__T('1'))!=string::npos)
            return 10000;
        if (Version.find(__T('2'))!=string::npos)
        {
            if (Profile.find(__T("Simple@"))!=string::npos)
            {
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20101;
                return 20100;
            }
            if (Profile.find(__T("Main@"))!=string::npos)
            {
                if (Profile.find(__T("Low"))!=string::npos)
                    return 20201;
                if (Profile.find(__T("@Main"))!=string::npos)
                    return 20202;
                if (Profile.find(__T("High 1440"))!=string::npos)
                    return 20203;
                if (Profile.find(__T("High"))!=string::npos)
                    return 20204;
                return 20200;
            }
            if (Profile.find(__T("SNR Scalable@"))!=string::npos)
            {
                if (Profile.find(__T("Low"))!=string::npos)
                    return 20301;
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20302;
                return 20300;
            }
            if (Profile.find(__T("Spatial Sclable@"))!=string::npos)
            {
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20401;
                if (Profile.find(__T("High 1440"))!=string::npos)
                    return 20402;
                if (Profile.find(__T("High"))!=string::npos)
                    return 20403;
                return 20400;
            }
            if (Profile.find(__T("High@"))!=string::npos)
            {
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20501;
                if (Profile.find(__T("High 1440"))!=string::npos)
                    return 20502;
                if (Profile.find(__T("High"))!=string::npos)
                    return 20503;
                return 20500;
            }
            if (Profile.find(__T("Multi-view@"))!=string::npos)
            {
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20601;
                return 20600;
            }
            if (Profile.find(__T("4:2:2@"))!=string::npos)
            {
                if (Profile.find(__T("Main"))!=string::npos)
                    return 20701;
                return 20700;
            }
            return 20000;
        }
    }
    if (Format==__T("MPEG-4 Visual"))
    {
        if (Profile.find(__T("Simple@"))==0)
        {
            if (Profile.find(__T("L0"))!=string::npos)
                return 30101;
            if (Profile.find(__T("L1"))!=string::npos)
                return 30102;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30103;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30104;
            if (Profile.find(__T("L4"))!=string::npos)
                return 30105;
            if (Profile.find(__T("L5"))!=string::npos)
                return 30106;
            return 30100;
        }
        if (Profile.find(__T("Simple Scalable@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 30201;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30202;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30203;
            return 30200;
        }
        if (Profile.find(__T("Advanced Simple@"))==0)
        {
            if (Profile.find(__T("L0"))!=string::npos)
                return 30301;
            if (Profile.find(__T("L1"))!=string::npos)
                return 30302;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30303;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30304;
            if (Profile.find(__T("L4"))!=string::npos)
                return 30305;
            if (Profile.find(__T("L5"))!=string::npos)
                return 30306;
            return 30100;
        }
        if (Profile.find(__T("Core@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 30401;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30402;
            return 30400;
        }
        if (Profile.find(__T("Core Scalable@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 30501;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30502;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30503;
            return 30500;
        }
        if (Profile.find(__T("Advanced Core@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 30601;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30602;
            return 30600;
        }
        if (Profile.find(__T("Main@"))==0)
        {
            if (Profile.find(__T("L2"))!=string::npos)
                return 30701;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30702;
            if (Profile.find(__T("L4"))!=string::npos)
                return 30703;
            return 30700;
        }
        if (Profile.find(__T("N-bit@"))==0)
        {
            if (Profile.find(__T("L2"))!=string::npos)
                return 30801;
            return 30800;
        }
        if (Profile.find(__T("Advanced Real Time Simple@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 30901;
            if (Profile.find(__T("L2"))!=string::npos)
                return 30902;
            if (Profile.find(__T("L3"))!=string::npos)
                return 30903;
            if (Profile.find(__T("L4"))!=string::npos)
                return 30904;
            return 30900;
        }
        if (Profile.find(__T("Advanced Coding Efficiency@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31001;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31002;
            if (Profile.find(__T("L3"))!=string::npos)
                return 31003;
            if (Profile.find(__T("L4"))!=string::npos)
                return 31004;
            return 31000;
        }
        if (Profile.find(__T("Simple Studio@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31101;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31102;
            if (Profile.find(__T("L3"))!=string::npos)
                return 31103;
            if (Profile.find(__T("L4"))!=string::npos)
                return 31104;
            return 31100;
        }
        if (Profile.find(__T("Core Studio@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31201;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31202;
            if (Profile.find(__T("L3"))!=string::npos)
                return 31203;
            if (Profile.find(__T("L4"))!=string::npos)
                return 31204;
            return 31200;
        }
        if (Profile.find(__T("Fine Granularity Scalable@"))==0)
        {
            if (Profile.find(__T("L0"))!=string::npos)
                return 31301;
            if (Profile.find(__T("L1"))!=string::npos)
                return 31302;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31303;
            if (Profile.find(__T("L3"))!=string::npos)
                return 31304;
            if (Profile.find(__T("L4"))!=string::npos)
                return 31305;
            if (Profile.find(__T("L5"))!=string::npos)
                return 31306;
            return 31300;
        }
        if (Profile.find(__T("Simple Face Animation@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31401;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31402;
            return 31400;
        }
        if (Profile.find(__T("Simple FBA@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31501;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31502;
            return 31500;
        }
        if (Profile.find(__T("Basic Animated Texture@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31601;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31602;
            return 31600;
        }
        if (Profile.find(__T("Scalable Texture@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31701;
            return 31700;
        }
        if (Profile.find(__T("Advanced Scalable Texture@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31801;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31802;
            if (Profile.find(__T("L3"))!=string::npos)
                return 31803;
            return 31800;
        }
        if (Profile.find(__T("Hybrid@"))==0)
        {
            if (Profile.find(__T("L1"))!=string::npos)
                return 31901;
            if (Profile.find(__T("L2"))!=string::npos)
                return 31902;
            return 31900;
        }
        return 30000;
    }
    if (Format==__T("JPEG"))
        return 50000;
    if (Format==__T("JPEG 2000"))
    {
        const Ztring &CodecID=MI.Get(Stream_Video, StreamPos, Video_CodecID);
        if (CodecID==__T("mjp2"))
            return 60100;
        if (CodecID==__T("mjs2"))
            return 60200;
        return 60000;
    }
    if (Format==__T("H.261"))
        return 70000;
    if (Format==__T("H.263"))
        return 80000;

    return 0;
}

//---------------------------------------------------------------------------
Ztring Mpeg7_Visual_colorDomain(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Colorimetry=MI.Get(Stream_Video, StreamPos, Video_Colorimetry);
    if (Colorimetry.find(__T("4:"))!=string::npos)
        return __T("color");
    if (Colorimetry==__T("Gray"))
        return __T("graylevel");
    return __T("");
}

//---------------------------------------------------------------------------
int32u Mpeg7_SystemCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    if (MI.Get(Stream_Video, StreamPos, Video_Standard)==__T("PAL"))
        return 10000;
    if (MI.Get(Stream_Video, StreamPos, Video_Standard)==__T("SECAM"))
        return 20000;
    if (MI.Get(Stream_Video, StreamPos, Video_Standard)==__T("NTSC"))
        return 30000;
    return 0;
}

Ztring Mpeg7_SystemCS_Name(int32u termID) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case  1 : return __T("PAL");
        case  2 : return __T("SECAM");
        case  3 : return __T("NTSC");
        default : return Ztring();
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_AudioCodingFormatCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Audio, StreamPos, Audio_Format);
    const Ztring &Version=MI.Get(Stream_Audio, StreamPos, Audio_Format_Version);
    const Ztring &Profile=MI.Get(Stream_Audio, StreamPos, Audio_Format_Profile);

    if (Format==__T("AC-3"))
        return 10000;
    if (Format==__T("DTS"))
        return 20000;
    if (Format==__T("MPEG Audio"))
    {
        if (Version.find(__T('1'))!=string::npos)
        {
            if (Profile.find(__T('1'))!=string::npos)
                return 30100;
            if (Profile.find(__T('2'))!=string::npos)
                return 30200;
            if (Profile.find(__T('3'))!=string::npos)
                return 30300;
            return 30000;
        }
        if (Version.find(__T('2'))!=string::npos)
        {
            if (Profile.find(__T('1'))!=string::npos)
                return 40100;
            if (Profile.find(__T('2'))!=string::npos)
                return 40200;
            if (Profile.find(__T('3'))!=string::npos)
                return 40300;
            return 40000;
        }
        return 0;
    }
    if (Format==__T("PCM"))
        return 80000;

    return 0;
}

Ztring Mpeg7_AudioCodingFormatCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case 1 : return __T("AC3");
        case 2 : return __T("DTS");
        case 3 :    switch ((termID%10000)/100)
                    {
                        case 1 : return __T("MPEG-1 Audio Layer I");
                        case 2 : return __T("MPEG-1 Audio Layer II");
                        case 3 : return __T("MPEG-1 Audio Layer III");
                        default: return __T("MPEG-1 Audio");
                    }
        case 4 :    switch ((termID%10000)/100)
                    {
                        case 1 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Audio Low Sampling Rate Layer I");
                                        case 2 : return __T("MPEG-2 Audio Low Sampling Rate Layer II");
                                        case 3 : return __T("MPEG-2 Audio Low Sampling Rate Layer III");
                                        default: return __T("MPEG-2 Audio Low Sampling Rate");
                                    }
                        case 2 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Backward Compatible Multi-Channel Layer I");
                                        case 2 : return __T("MPEG-2 Backward Compatible Multi-Channel Layer II");
                                        case 3 : return __T("MPEG-2 Backward Compatible Multi-Channel Layer III");
                                        default: return __T("MPEG-2 Backward Compatible Multi-Channel");
                                    }
                        default: return __T("MPEG-2 Audio");
                    }
        case 8 : return __T("Linear PCM");
        default: return MI.Get(Stream_Audio, StreamPos, Video_Format);
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_AudioPresentationCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Channels=MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_);
    const Ztring &ChannelPositions2=MI.Get(Stream_Audio, StreamPos, Audio_ChannelPositions_String2);
    if (Channels==__T("6") && ChannelPositions2==__T("3/2.1"))
        return 50000;
    if (Channels==__T("8") && ChannelPositions2==__T("3/2/2.1"))
        return 60000;
    if (Channels==__T("2"))
        return 30000;
    if (Channels==__T("1"))
        return 20000;
    return 0;
}

Ztring Mpeg7_AudioPresentationCS_Name(int32u termID, MediaInfo_Internal &, size_t)
{
    switch (termID/10000)
    {
        case 2 : return __T("mono");
        case 3 : return __T("stereo");
        case 5 : return __T("Home theater 5.1");
        case 6 : return __T("Movie theater");
        default: return Ztring();
    }
}

//---------------------------------------------------------------------------
Ztring Mpeg7_AudioEmphasis(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Emphasis=MI.Get(Stream_Audio, StreamPos, Audio_Format_Settings_Emphasis);
    if (Emphasis==__T("50/15ms"))
        return __T("50over15Microseconds");
    if (Emphasis==__T("CCITT"))
        return __T("ccittJ17");
    if (Emphasis==__T("Reserved"))
        return __T("reserved");
    return __T("none");
}

//---------------------------------------------------------------------------
Ztring Mpeg7_MediaTimePoint(MediaInfo_Internal &MI)
{
    if (MI.Count_Get(Stream_Video)==1 && MI.Get(Stream_General, 0, General_Format)==__T("MPEG-PS"))
    {
        int64u Delay=(int64u)(MI.Get(Stream_Video, 0, Video_Delay).To_float64()*90);
        int64u Rate=90000;
        int64u DD=Delay/(24*60*60*Rate);
        Delay=Delay%(24*60*60*Rate);
        int64u HH=Delay/(60*60*Rate);
        Delay=Delay%(60*60*Rate);
        int64u MM=Delay/(60*Rate);
        Delay=Delay%(60*Rate);
        int64u Sec=Delay/Rate;
        Delay=Delay%Rate;
        Ztring ToReturn;
        if (DD)
            ToReturn+=Ztring::ToZtring(DD);
        ToReturn+=__T('T');
        ToReturn+=(HH<10?__T("0"):__T(""))+Ztring::ToZtring(HH)+__T(':');
        ToReturn+=(MM<10?__T("0"):__T(""))+Ztring::ToZtring(MM)+__T(':');
        ToReturn+=(Sec<10?__T("0"):__T(""))+Ztring::ToZtring(Sec)+__T(':');
        ToReturn+=Ztring::ToZtring(Delay)+__T('F');
        ToReturn+=Ztring::ToZtring(Rate);
        return ToReturn;
    }

    //Default: In milliseconds
    int64u Milliseconds=MI.Get(Stream_Video, 0, Video_Delay).To_int64u();
    int64u DD=Milliseconds/(24*60*60*1000);
    Milliseconds=Milliseconds%(24*60*60*1000);
    int64u HH=Milliseconds/(60*60*1000);
    Milliseconds=Milliseconds%(60*60*1000);
    int64u MM=Milliseconds/(60*1000);
    Milliseconds=Milliseconds%(60*1000);
    int64u Sec=Milliseconds/1000;
    int64u NN=Milliseconds%1000;
    int64u FF=1000;
    Ztring ToReturn;
    if (DD)
        ToReturn+=Ztring::ToZtring(DD);
    ToReturn+=__T('T');
    ToReturn+=(HH<10?__T("0"):__T(""))+Ztring::ToZtring(HH)+__T(':');
    ToReturn+=(MM<10?__T("0"):__T(""))+Ztring::ToZtring(MM)+__T(':');
    ToReturn+=(Sec<10?__T("0"):__T(""))+Ztring::ToZtring(Sec)+__T(':');
    ToReturn+=Ztring::ToZtring(NN)+__T('F');
    ToReturn+=Ztring::ToZtring(FF);
    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Mpeg7_MediaDuration(MediaInfo_Internal &MI)
{
    if (MI.Count_Get(Stream_Video)==1)
    {
        int64u FrameCount=MI.Get(Stream_Video, 0, Video_FrameCount).To_int64u();
        int64u FrameRate=MI.Get(Stream_Video, 0, Video_FrameRate).To_int64u();
        if (FrameRate==0)
            return Ztring();
        int64u DD=FrameCount/(24*60*60*FrameRate);
        FrameCount=FrameCount%(24*60*60*FrameRate);
        int64u HH=FrameCount/(60*60*FrameRate);
        FrameCount=FrameCount%(60*60*FrameRate);
        int64u MM=FrameCount/(60*FrameRate);
        FrameCount=FrameCount%(60*FrameRate);
        int64u Sec=FrameCount/FrameRate;
        FrameCount=FrameCount%FrameRate;
        Ztring ToReturn;
        ToReturn+=__T('P');
        if (DD)
            ToReturn+=Ztring::ToZtring(DD)+__T('D');
        ToReturn+=__T('T');
        ToReturn+=Ztring::ToZtring(HH)+__T('H');
        ToReturn+=Ztring::ToZtring(MM)+__T('M');
        ToReturn+=Ztring::ToZtring(Sec)+__T('S');
        ToReturn+=Ztring::ToZtring(FrameCount)+__T('N');
        ToReturn+=Ztring::ToZtring(FrameRate)+__T('F');
        return ToReturn;
    }

    if (MI.Count_Get(Stream_Audio)==1)
    {
        int64u SamplingCount=MI.Get(Stream_Audio, 0, Audio_SamplingCount).To_int64u();
        int64u SamplingRate=MI.Get(Stream_Audio, 0, Audio_SamplingRate).To_int64u();
        if (SamplingRate==0)
            return Ztring();
        int64u DD=SamplingCount/(24*60*60*SamplingRate);
        SamplingCount=SamplingCount%(24*60*60*SamplingRate);
        int64u HH=SamplingCount/(60*60*SamplingRate);
        SamplingCount=SamplingCount%(60*60*SamplingRate);
        int64u MM=SamplingCount/(60*SamplingRate);
        SamplingCount=SamplingCount%(60*SamplingRate);
        int64u Sec=SamplingCount/SamplingRate;
        SamplingCount=SamplingCount%SamplingRate;
        Ztring ToReturn;
        ToReturn+=__T('P');
        if (DD)
            ToReturn+=Ztring::ToZtring(DD)+__T('D');
        ToReturn+=__T('T');
        ToReturn+=Ztring::ToZtring(HH)+__T('H');
        ToReturn+=Ztring::ToZtring(MM)+__T('M');
        ToReturn+=Ztring::ToZtring(Sec)+__T('S');
        ToReturn+=Ztring::ToZtring(SamplingCount)+__T('N');
        ToReturn+=Ztring::ToZtring(SamplingRate)+__T('F');
        return ToReturn;
    }

    //Default: In milliseconds
    int64u Milliseconds=MI.Get(Stream_General, 0, General_Duration).To_int64u();
    int64u DD=Milliseconds/(24*60*60*1000);
    Milliseconds=Milliseconds%(24*60*60*1000);
    int64u HH=Milliseconds/(60*60*1000);
    Milliseconds=Milliseconds%(60*60*1000);
    int64u MM=Milliseconds/(60*1000);
    Milliseconds=Milliseconds%(60*1000);
    int64u Sec=Milliseconds/1000;
    int64u NN=Milliseconds%1000;
    int64u FF=1000;
    Ztring ToReturn;
    ToReturn+=__T('P');
    if (DD)
        ToReturn+=Ztring::ToZtring(DD)+__T('D');
    ToReturn+=__T('T');
    ToReturn+=Ztring::ToZtring(HH)+__T('H');
    ToReturn+=Ztring::ToZtring(MM)+__T('M');
    ToReturn+=Ztring::ToZtring(Sec)+__T('S');
    ToReturn+=Ztring::ToZtring(NN)+__T('N');
    ToReturn+=Ztring::ToZtring(FF)+__T('F');
    return ToReturn;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Export_Mpeg7::Export_Mpeg7 ()
{
}

//---------------------------------------------------------------------------
Export_Mpeg7::~Export_Mpeg7 ()
{
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
void Mpeg7_Transform_Visual(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos)
{
    Node* Node_VisualCoding=Parent->Add_Child("mpeg7:VisualCoding");

    //Format
    Node* Node_Format=Node_VisualCoding->Add_Child("mpeg7:Format");

    int32u VisualCodingFormatCS_termID=Mpeg7_VisualCodingFormatCS_termID(MI, StreamPos);
    if (VisualCodingFormatCS_termID)
        Node_Format->Add_Attribute("href", __T("urn:mpeg:mpeg7:cs:VisualCodingFormatCS:2001:")+Ztring::ToZtring(VisualCodingFormatCS_termID/10000));
    else
        Node_Format->Add_Attribute("href", __T("urn:x-mpeg7-mediainfo:cs:VisualCodingFormatCS:2009:unknown"));

    Ztring Value=Mpeg7_Visual_colorDomain(MI, StreamPos); //Algo puts empty string if not known
    if (!Value.empty())
        Node_Format->Add_Attribute("colorDomain", Value);

    Node_Format->Add_Child("mpeg7:Name", VideoCompressionCodeCS_Name((VisualCodingFormatCS_termID/10000)*10000, MI, StreamPos), "xml:lang", "en");
    if (VisualCodingFormatCS_termID%10000)
    {
        Node* Node_Term=Node_Format->Add_Child("mpeg7:Term", "", "termID", Ztring::ToZtring(VisualCodingFormatCS_termID/10000)+__T(".")+
                                                                       Ztring::ToZtring((VisualCodingFormatCS_termID%10000)/100));
        Node_Term->Add_Child("mpeg7:Name", VideoCompressionCodeCS_Name((VisualCodingFormatCS_termID/100)*100, MI, StreamPos), "xml:lang", "en");
        if (VisualCodingFormatCS_termID%100)
        {
            Node* Node_Term2=Node_Term->Add_Child("mpeg7:Term", "", "termID", Ztring::ToZtring(VisualCodingFormatCS_termID/10000)+__T(".")+
                                                                            Ztring::ToZtring((VisualCodingFormatCS_termID%10000)/100)+__T(".")+
                                                                            Ztring::ToZtring(VisualCodingFormatCS_termID%100));
            Node_Term2->Add_Child("mpeg7:Name", VideoCompressionCodeCS_Name(VisualCodingFormatCS_termID, MI, StreamPos), "xml:lang", "en");
        }
    }

    //Pixel
    if (!MI.Get(Stream_Video, 0, Video_PixelAspectRatio).empty()
     || !MI.Get(Stream_Video, 0, Video_Resolution).empty())
    {
        Node* Node_Pixel=Node_VisualCoding->Add_Child("mpeg7:Pixel");
        Node_Pixel->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_PixelAspectRatio, "aspectRatio");
        Node_Pixel->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_Resolution, "bitsPer");
    }

    //Frame
    if (!MI.Get(Stream_Video, 0, Video_DisplayAspectRatio).empty()
     || !MI.Get(Stream_Video, 0, Video_Height).empty()
     || !MI.Get(Stream_Video, 0, Video_Width).empty()
     || !MI.Get(Stream_Video, 0, Video_FrameRate).empty()
     || !MI.Get(Stream_Video, 0, Video_ScanType).empty())
    {
        Node* Node_Frame=Node_VisualCoding->Add_Child("mpeg7:Frame");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_DisplayAspectRatio, "aspectRatio");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_Height, "height");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_Width, "width");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, 0, Video_FrameRate, "rate");

        Value=MI.Get(Stream_Video, 0, Video_ScanType).MakeLowerCase();
        if (!Value.empty())
            Node_Frame->Add_Attribute("structure", Value==__T("mbaff")?Ztring(__T("interlaced")):Value);
    }

    //Colorimetry
    if (MI.Get(Stream_Video, StreamPos, Video_Colorimetry).find(__T("4:2:0"))!=string::npos)
    {
        Node* Node_ColorSampling=Node_VisualCoding->Add_Child("mpeg7:ColorSampling");
        Node* Node_Lattice=Node_ColorSampling->Add_Child("mpeg7:Lattice");
        Node_Lattice->Add_Attribute("height", "720");
        Node_Lattice->Add_Attribute("width", "486");

        {
            Node* Node_Field=Node_ColorSampling->Add_Child("mpeg7:Field");
            Node_Field->Add_Attribute("temporalOrder", "0");
            Node_Field->Add_Attribute("positionalOrder", "0");

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("Luminance"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "0.0");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "1.0");
                Node_Period->Add_Attribute("vertical", "2.0");
            }

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("ChrominanceBlueDifference"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "0.5");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "2.0");
                Node_Period->Add_Attribute("vertical", "4.0");
            }

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("ChrominanceRedDifference"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "0.5");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "2.0");
                Node_Period->Add_Attribute("vertical", "4.0");
            }
        }

        {
            Node* Node_Field=Node_ColorSampling->Add_Child("mpeg7:Field");
            Node_Field->Add_Attribute("temporalOrder", "1");
            Node_Field->Add_Attribute("positionalOrder", "1");

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("Luminance"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "1.0");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "1.0");
                Node_Period->Add_Attribute("vertical", "2.0");
            }

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("ChrominanceBlueDifference"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "2.5");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "2.0");
                Node_Period->Add_Attribute("vertical", "4.0");
            }

            {
                Node* Node_Component=Node_Field->Add_Child("mpeg7:Component");
                Node_Component->Add_Child("mpeg7:Name", std::string("ChrominanceRedDifference"));
                Node* Node_Offset=Node_Component->Add_Child("mpeg7:Offset");
                Node_Offset->Add_Attribute("horizontal", "0.0");
                Node_Offset->Add_Attribute("vertical", "2.5");
                Node* Node_Period=Node_Component->Add_Child("mpeg7:Period");
                Node_Period->Add_Attribute("horizontal", "4.0");
                Node_Period->Add_Attribute("vertical", "2.0");
            }
        }
    }
}

//---------------------------------------------------------------------------
void Mpeg7_Transform_Audio(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos)
{
    Node* Node_AudioCoding=Parent->Add_Child("mpeg7:AudioCoding");

    //Format
    //Format
    Node* Node_Format=Node_AudioCoding->Add_Child("mpeg7:Format");

    int32u AudioCodingFormatCS_termID=Mpeg7_AudioCodingFormatCS_termID(MI, StreamPos);
    if (AudioCodingFormatCS_termID)
        Node_Format->Add_Attribute("href", __T("urn:mpeg:mpeg7:cs:AudioCodingFormatCS:2001:")+Ztring::ToZtring(AudioCodingFormatCS_termID/10000));
    else
       Node_Format->Add_Attribute("href", __T("urn:x-mpeg7-mediainfo:cs:AudioCodingFormatCS:2009:unknown"));

    Node_Format->Add_Child("mpeg7:Name", Mpeg7_AudioCodingFormatCS_Name((AudioCodingFormatCS_termID/10000)*10000, MI, StreamPos), "xml:lang", "en");
    if (AudioCodingFormatCS_termID%10000)
    {
        Node* Node_Term=Node_Format->Add_Child("mpeg7:Term", "", "termID", Ztring::ToZtring(AudioCodingFormatCS_termID/10000)+__T(".")+
                                                                       Ztring::ToZtring((AudioCodingFormatCS_termID%10000)/100));
        Node_Term->Add_Child("mpeg7:Name", Mpeg7_AudioCodingFormatCS_Name((AudioCodingFormatCS_termID/100)*100, MI, StreamPos), "xml:lang", "en");
        if (AudioCodingFormatCS_termID%100)
        {
            Node* Node_Term2=Node_Term->Add_Child("mpeg7:Term", "", "termID", Ztring::ToZtring(AudioCodingFormatCS_termID/10000)+__T(".")+
                                                                            Ztring::ToZtring((AudioCodingFormatCS_termID%10000)/100)+__T(".")+
                                                                            Ztring::ToZtring(AudioCodingFormatCS_termID%100));
            Node_Term2->Add_Child("mpeg7:Name", Mpeg7_AudioCodingFormatCS_Name(AudioCodingFormatCS_termID, MI, StreamPos), "xml:lang", "en");
        }
    }

    //AudioChannels
    Node_AudioCoding->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Channel_s_, "mpeg7:AudioChannels");

    //Sample
    Node* Node_Sample=Node_AudioCoding->Add_Child("mpeg7:Sample");
    Node_Sample->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_SamplingRate, "rate");
    Node_Sample->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_BitDepth, "bitsPer");

    //Emphasis
    if (MI.Get(Stream_Audio, StreamPos, Audio_Format)==__T("MPEG Audio"))
        Node_AudioCoding->Add_Child("mpeg7:Emphasis", Mpeg7_AudioEmphasis(MI, StreamPos));

    //Presentation
    int32u AudioPresentationCS_termID=Mpeg7_AudioPresentationCS_termID(MI, StreamPos);
    if (AudioPresentationCS_termID)
    {
        Node* Node_Presentation=Node_AudioCoding->Add_Child("mpeg7:Presentation");
        Ztring Value=__T("urn:mpeg:mpeg7:cs:AudioPresentationCS:2001:");
        Value+=Ztring::ToZtring(AudioPresentationCS_termID/10000);
        if (AudioPresentationCS_termID%10000)
        {
            Value+=__T(".");
            Value+=Ztring::ToZtring((AudioPresentationCS_termID%10000)/100);
            if (AudioPresentationCS_termID%100)
            {
                Value+=__T(".");
                Value+=Ztring::ToZtring(AudioPresentationCS_termID%100);
            }
        }
        Node_Presentation->Add_Attribute("href", Value);

        Node_Presentation->Add_Child("mpeg7:Name", Mpeg7_AudioPresentationCS_Name(AudioPresentationCS_termID, MI, StreamPos));
    }
}

//---------------------------------------------------------------------------
Ztring Export_Mpeg7::Transform(MediaInfo_Internal &MI)
{
    Ztring Value;

    //ebuCoreMain
    Node* Node_Mpeg7 = new Node("mpeg7:Mpeg7");
    Node_Mpeg7->Add_Attribute("xmlns", "urn:mpeg:mpeg7:schema:2004");
    Node_Mpeg7->Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    Node_Mpeg7->Add_Attribute("xmlns:mpeg7", "urn:mpeg:mpeg7:schema:2004");
    Node_Mpeg7->Add_Attribute("xsi:schemaLocation", "urn:mpeg:mpeg7:schema:2004 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-7_schema_files/mpeg7-v2.xsd");

    //Description - DescriptionMetadata
    Node* Node_DescriptionMetadata=Node_Mpeg7->Add_Child("mpeg7:DescriptionMetadata");

    Node_DescriptionMetadata->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_ISRC, "mpeg7:PublicIdentifier", "type", std::string("ISRC"));

    Ztring FileName=MI.Get(Stream_General, 0, General_FileName);
    Ztring Extension=MI.Get(Stream_General, 0, General_FileExtension);
    if (!Extension.empty())
        FileName+=__T('.')+Extension;
    if (!FileName.empty())
       Node_DescriptionMetadata->Add_Child("mpeg7:PrivateIdentifier", FileName);

    //Current date/time is ISO format
    time_t Time=time(NULL);
    Ztring TimeS; TimeS.Date_From_Seconds_1970((int32u)Time);
    TimeS.FindAndReplace(__T("UTC "), __T(""));
    TimeS.FindAndReplace(__T(" "), __T("T"));
    TimeS+=__T("+00:00");
    Node_DescriptionMetadata->Add_Child("mpeg7:CreationTime", TimeS);

    Node* Node_Instrument=Node_DescriptionMetadata->Add_Child("mpeg7:Instrument");
    Node_Instrument->Add_Child("mpeg7:Tool")->Add_Child("mpeg7:Name", MediaInfoLib::Config.Info_Version_Get());

    //Description - CreationDescription
    Node* Node_Description=Node_Mpeg7->Add_Child("mpeg7:Description", "", "xsi:type", "ContentEntityType");

    //MultimediaContent
    Node* Node_MultimediaContent=Node_Description->Add_Child("mpeg7:MultimediaContent", std::string(""), "xsi:type", Ztring(Ztring(Mpeg7_Type(MI))+__T("Type")).To_UTF8());

    //(Type)
    Node* Node_Type=Node_MultimediaContent->Add_Child(Ztring(Ztring(__T("mpeg7:"))+Ztring(Mpeg7_Type(MI))).To_UTF8());

    //MediaFormat header
    Node* Node_MediaInformation=Node_Type->Add_Child("mpeg7:MediaInformation");
    Node* Node_MediaProfile=Node_MediaInformation->Add_Child("mpeg7:MediaProfile");

    Node* Node_MediaFormat=Node_MediaProfile->Add_Child("mpeg7:MediaFormat");

    //Content
    Node* Node_Content=Node_MediaFormat->Add_Child("mpeg7:Content");

    int32u ContentCS_termID=Mpeg7_ContentCS_termID(MI);
    if (ContentCS_termID>=500000) //Out of spec
        Value=__T("urn:x-mpeg7-mediainfo:cs:ContentCS:2009:");
    else
        Value=__T("urn:mpeg:mpeg7:cs:ContentCS:2001:");

    Value+=Ztring::ToZtring(ContentCS_termID/10000);
    if (ContentCS_termID%10000)
    {
        Value+=__T(".");
        Value+=Ztring::ToZtring((ContentCS_termID%10000)/100);
        if (ContentCS_termID%100)
        {
            Value+=__T(".");
            Value+=Ztring::ToZtring(ContentCS_termID%100);
        }
    }

    Node_Content->Add_Attribute("href", Value);
    Node_Content->Add_Child("mpeg7:Name", Mpeg7_ContentCS_Name(ContentCS_termID, MI), "xml:lang", std::string("en"));

    //FileFormat
    if (!MI.Get(Stream_General, 0, General_Format).empty())
    {
        Node* Node_FileFormat=Node_MediaFormat->Add_Child("mpeg7:FileFormat");

        Value="urn:x-mpeg7-mediainfo:cs:FileFormatCS:2009:unknown";
        int32u FileFormatCS_termID=Mpeg7_FileFormatCS_termID(MI);
        if (FileFormatCS_termID)
        {
            if (FileFormatCS_termID>=500000) //Out of spec
                Value="urn:x-mpeg7-mediainfo:cs:FileFormatCS:2009:";
            else
                Value="urn:mpeg:mpeg7:cs:FileFormatCS:2001:";
            Value+=Ztring::ToZtring(FileFormatCS_termID/10000);
        }
        Node_FileFormat->Add_Attribute("href", Value);

        Node_FileFormat->Add_Child("mpeg7:Name", Mpeg7_FileFormatCS_Name((FileFormatCS_termID/10000)*10000, MI), "xml:lang", "en");
        if (FileFormatCS_termID%10000)
        {
            Node* Node_Term=Node_FileFormat->Add_Child("mpeg7:Term");
            Node_Term->Add_Attribute("termID", Ztring::ToZtring(FileFormatCS_termID/10000)+__T(".")+
                                               Ztring::ToZtring((FileFormatCS_termID%10000)/100));

            Node_Term->Add_Child("mpeg7:Name", Mpeg7_FileFormatCS_Name((FileFormatCS_termID/100)*100, MI), "xml:lang", "en");
            if (FileFormatCS_termID%100)
            {
                Node* Node_Term2=Node_Term->Add_Child("mpeg7:Term");
                Node_Term2->Add_Attribute("termID", Ztring::ToZtring(FileFormatCS_termID/10000)+__T(".")+
                                                    Ztring::ToZtring((FileFormatCS_termID%10000)/100)+__T(".")+
                                                    Ztring::ToZtring(FileFormatCS_termID%100));

                Node_Term2->Add_Child("mpeg7:Name", Mpeg7_FileFormatCS_Name(FileFormatCS_termID, MI), "xml:lang", "en");
            }
        }
    }

    //FileSize
    Node_MediaFormat->Add_Child("mpeg7:FileSize", MI.Get(Stream_General, 0, General_FileSize));

    //System
    if (!MI.Get(Stream_Video, 0, Video_Standard).empty())
    {
        Node* Node_System=Node_MediaFormat->Add_Child("mpeg7:System");
        int32u SystemCS_termID=Mpeg7_SystemCS_termID(MI, 0); //2 video streams are not supported
        if (SystemCS_termID)
        {
            Value="urn:mpeg:mpeg7:cs:SystemCS:2001:";
            Value+=Ztring::ToZtring(SystemCS_termID/10000);
            if (SystemCS_termID%10000)
            {
                Value+=__T(".");
                Value+=Ztring::ToZtring((SystemCS_termID%10000)/100);
                if (SystemCS_termID%100)
                {
                    Value+=__T(".");
                    Value+=Ztring::ToZtring(SystemCS_termID%100);
                }
            }
            Node_System->Add_Attribute("href", Value);
        }
        Node_System->Add_Child("mpeg7:Name", Mpeg7_SystemCS_Name(SystemCS_termID), "xml:lang", "en");
    }

    //BitRate
    if (!MI.Get(Stream_General, 0, General_OverallBitRate).empty())
    {
        Node* Node_BitRate=Node_MediaFormat->Add_Child("mpeg7:BitRate", MI.Get(Stream_General, 0, General_OverallBitRate));
        bool IsCBR=true;
        bool IsVBR=true;
        for (size_t StreamKind=Stream_Video; StreamKind<=Stream_Audio; StreamKind++)
            for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
            {
                if (IsCBR && MI.Get((stream_t)StreamKind, StreamPos, __T("BitRate_Mode"))==__T("VBR"))
                    IsCBR=false;
                if (IsVBR && MI.Get((stream_t)StreamKind, StreamPos, __T("BitRate_Mode"))==__T("CBR"))
                    IsVBR=false;
            }
        if (IsCBR && IsVBR)
        {
            IsCBR=false;
            IsVBR=false;
        }
        if (IsCBR)
            Node_BitRate->Add_Attribute("variable", "false");
        if (IsVBR)
            Node_BitRate->Add_Attribute("variable", "true");
    }

    //xxxCoding
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Video); Pos++)
        Mpeg7_Transform_Visual(Node_MediaFormat, MI, Pos);
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Audio); Pos++)
        Mpeg7_Transform_Audio(Node_MediaFormat, MI, Pos);

    //MediaTranscodingHints, intraFrameDistance and anchorFrameDistance
    if (!MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).empty())
    {
        Ztring M=MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).SubString(__T("M="), __T(","));
        Ztring N=MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).SubString(__T("N="), __T(""));

        Node* Node_CodingHints=Node_MediaProfile->Add_Child("mpeg7:MediaTranscodingHints")->Add_Child("mpeg7:CodingHints");
        if (!N.empty())
            Node_CodingHints->Add_Attribute("intraFrameDistance", N);
        if (!M.empty())
            Node_CodingHints->Add_Attribute("anchorFrameDistance", M);
    }

    if (!MI.Get(Stream_General, 0, General_Movie).empty()
     || !MI.Get(Stream_General, 0, General_Track).empty()
     || !MI.Get(Stream_General, 0, General_Track_Position).empty()
     || !MI.Get(Stream_General, 0, General_Album).empty()
     || !MI.Get(Stream_General, 0, General_Encoded_Library).empty()
     || !MI.Get(Stream_General, 0, General_Performer).empty())
    {
        Node* Node_Creation=Node_Type->Add_Child("mpeg7:CreationInformation")->Add_Child("mpeg7:Creation");

        if (!MI.Get(Stream_General, 0, General_Movie).empty()
         || !MI.Get(Stream_General, 0, General_Track).empty()
         || !MI.Get(Stream_General, 0, General_Track_Position).empty()
         || !MI.Get(Stream_General, 0, General_Album).empty())
        {
            Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Movie, "mpeg7:Title", "type", std::string("songTitle"));
            Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Track, "mpeg7:Title", "type", std::string("songTitle"));
            if (!MI.Get(Stream_General, 0, General_Track_Position).empty())
            {
                 Ztring Total=MI.Get(Stream_General, 0, General_Track_Position_Total);
                 Value=MI.Get(Stream_General, 0, General_Track_Position)+(Total.empty()?Ztring():(__T("/")+Total));

                 Node_Creation->Add_Child("mpeg7:Title", Value, "type", std::string("urn:x-mpeg7-mediainfo:cs:TitleTypeCS:2009:TRACK"));
            }
            Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Album, "mpeg7:Title", "type", std::string("albumTitle"));
        }
        else
        {
              Node_Creation->Add_Child("mpeg7:Title", FileName);
        }

        if (!MI.Get(Stream_General, 0, General_WrittenBy).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:AUTHOR");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_WrittenBy));
        }

        if (!MI.Get(Stream_General, 0, General_Performer).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:PERFORMER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Performer));
        }
        if (!MI.Get(Stream_General, 0, General_ExecutiveProducer).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:EXECUTIVE-PRODUCER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_ExecutiveProducer));
        }
        if (!MI.Get(Stream_General, 0, General_Producer).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:PRODUCER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Producer));
        }
        if (!MI.Get(Stream_General, 0, General_Director).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:PRODUCER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Director));
        }
        if (!MI.Get(Stream_General, 0, General_Composer).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:COMPOSER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Composer));
        }
        if (!MI.Get(Stream_General, 0, General_CostumeDesigner).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:COSTUME-SUPERVISOR");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_CostumeDesigner));
        }
        if (!MI.Get(Stream_General, 0, General_ProductionDesigner).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:PRODUCTION-DESIGNER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_ProductionDesigner));
        }
        if (!MI.Get(Stream_General, 0, General_Publisher).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:PUBLISHER");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Publisher));
        }
        if (!MI.Get(Stream_General, 0, General_DistributedBy).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:mpeg:mpeg7:cs:RoleCS:2001:DISTRIBUTOR");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "PersonGroupType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_DistributedBy));
        }
        if (!MI.Get(Stream_General, 0, General_Label).empty())
        {
            Node* Node_Creator=Node_Creation->Add_Child("mpeg7:Creator");
            Node_Creator->Add_Child("mpeg7:Role", "", "href", "urn:x-mpeg7-mediainfo:cs:RoleCS:2009:RECORD-LABEL");
            Node* Node_Agent=Node_Creator->Add_Child("mpeg7:Agent", "", "xsi:type", "OrganizationType");
            Node_Agent->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Label));
        }
        if (!MI.Get(Stream_General, 0, General_Encoded_Library).empty())
        {
            Node* Node_Tool=Node_Creation->Add_Child("mpeg7:CreationTool")->Add_Child("mpeg7:Tool");
            Node_Tool->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Encoded_Library));
        }
    }

    if (MI.Count_Get(Stream_Video)==1 || MI.Count_Get(Stream_Audio)==1)
    {
        Node* Node_MediaTime=Node_Type->Add_Child("mpeg7:MediaTime");
        //MediaTimePoint
        Value=Mpeg7_MediaTimePoint(MI);
        if (!Value.empty())
            Node_MediaTime->Add_Child("mpeg7:MediaTimePoint", Value);
        //MediaDuration
        Value=Mpeg7_MediaDuration(MI);
        if (!Value.empty())
            Node_MediaTime->Add_Child("mpeg7:MediaDuration", Value);
    }

    Ztring ToReturn=Ztring().From_UTF8(To_XML(*Node_Mpeg7, 0, true, true).c_str());

    //Find and replace
    ZtringListList ToReplace=MediaInfoLib::Config.Inform_Replace_Get_All();
    for (size_t Pos=0; Pos<ToReplace.size(); Pos++)
        ToReturn.FindAndReplace(ToReplace[Pos][0], ToReplace[Pos][1], 0, Ztring_Recursive);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif

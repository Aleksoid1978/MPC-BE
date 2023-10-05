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
#include <algorithm>

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
const size_t FieldsToOffset_Size=5;
extern const char* FieldsToOffset[FieldsToOffset_Size];

//---------------------------------------------------------------------------
static Ztring Mpeg7_TimeToISO(Ztring Value)
{
    if (Value.find(__T(" - ")) != string::npos)
    {
        ZtringList List;
        List.Separator_Set(0, __T(" - "));
        List.Write(Value);
        Value = List[0];
        for (size_t i = 1; i < List.size(); i++)
        {
            if (Value > List[i])
                Value = List[i];
        }
    }

    if (Value.size()>=4 && Value.find(__T(" UTC"), Value.size()-4)!=string::npos)
    {
        Value.erase(Value.size()-4);
        Value+=__T("+00:00");
    }
    if (Value.size()>11 && Value[10]==__T(' '))
        Value[10]=__T('T');
    if (Value.size() > 19 && Value[19] == __T('.')) //Milliseconds are valid ISO but MPEG-7 XSD does not like them
        Value.erase(19, Value.find_first_not_of(__T("0123456789"), 20)-19);
    return Value;
}

//---------------------------------------------------------------------------
static bool Mpeg7_TimeToISO_Isvalid(const Ztring& TimePoint)
{
    if (TimePoint.size()<=3)
       return false;
    else if (!(TimePoint[0]>=__T('0') && TimePoint[0]<=__T('9')
            && TimePoint[1]>=__T('0') && TimePoint[1]<=__T('9')
            && TimePoint[2]>=__T('0') && TimePoint[2]<=__T('9')
            && TimePoint[3]>=__T('0') && TimePoint[3]<=__T('9')))
       return false;
    else if (TimePoint.size()>4)
    {
        if (TimePoint.size()<=6)
           return false;
        else if (!(TimePoint[4]==__T('-')
                && TimePoint[5]>=__T('0') && TimePoint[5]<=__T('9')
                && TimePoint[6]>=__T('0') && TimePoint[6]<=__T('9')))
           return false;
        else if (TimePoint.size()>7)
        {
            if (TimePoint.size()<=9)
               return false;
            else if (!(TimePoint[7]==__T('-')
                    && TimePoint[8]>=__T('0') && TimePoint[8]<=__T('9')
                    && TimePoint[9]>=__T('0') && TimePoint[9]<=__T('9')))
               return false;
            else if (TimePoint.size()>10)
            {
                if (TimePoint.size()<=12)
                   return false;
                else if (!(TimePoint[10]==__T('T')
                        && TimePoint[11]>=__T('0') && TimePoint[11]<=__T('9')
                        && TimePoint[12]>=__T('0') && TimePoint[12]<=__T('9')))
                   return false;
                else if (TimePoint.size()>13)
                {
                    if (TimePoint.size()<=15)
                       return false;
                    else if (!(TimePoint[13]==__T(':')
                            && TimePoint[14]>=__T('0') && TimePoint[14]<=__T('9')
                            && TimePoint[15]>=__T('0') && TimePoint[15]<=__T('9')))
                       return false;
                    else if (TimePoint.size()>16)
                    {
                        if (TimePoint.size()<=18)
                           return false;
                        else if (!(TimePoint[16]==__T(':')
                                && TimePoint[17]>=__T('0') && TimePoint[17]<=__T('9')
                                && TimePoint[18]>=__T('0') && TimePoint[18]<=__T('9')))
                           return false;
                        else if (TimePoint.size()>19)
                        {
                            if (TimePoint.size()<=24)
                               return false;
                            else if (!((TimePoint[19]==__T('+') || TimePoint[19]==__T('-'))
                                    && TimePoint[20]>=__T('0') && TimePoint[20]<=__T('9')
                                    && TimePoint[21]>=__T('0') && TimePoint[21]<=__T('9')
                                    && TimePoint[22]>=__T(':')
                                    && TimePoint[23]>=__T('0') && TimePoint[23]<=__T('9')
                                    && TimePoint[24]>=__T('0') && TimePoint[24]<=__T('9')))
                               return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------
const Char* Mpeg7_Type(MediaInfo_Internal &MI) //TO ADAPT
{
    if (MI.Count_Get(Stream_Image)>1)
        return __T("Multimedia");
    else if (MI.Count_Get(Stream_Video))
    {
        if (MI.Count_Get(Stream_Audio))
            return __T("AudioVisual");
        else
            return __T("Video");
    }
    else if (MI.Count_Get(Stream_Audio))
        return __T("Audio");
    else if (MI.Count_Get(Stream_Image))
        return __T("Image");
    else if (MI.Count_Get(Stream_Text))
        return __T("AudioVisual"); //"Text" does not permit MediaTime

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
int32u Mpeg7_ContentCS_termID(MediaInfo_Internal &MI, size_t)
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
    else if (MI.Count_Get(Stream_Text))
        return 500000;

    //Not known
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);
    if (Format==__T("AVI") || Format==__T("DV") || Format==__T("MPEG-4") || Format==__T("MPEG-PS") || Format==__T("MPEG-TS") || Format==__T("QuickTime") || Format==__T("Windows Media"))
        return 40200;
    if (Format==__T("MPEG Audio") || Format==__T("Wave"))
        return 10000;
    if (Format==__T("BMP") || Format==__T("GIF") || Format==__T("JPEG") || Format==__T("JPEG 2000") || Format==__T("PNG") || Format==__T("TIFF"))
        return 40100;
    return 0;
}

Ztring Mpeg7_ContentCS_Name(int32u termID, MediaInfo_Internal &MI, size_t) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
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
        case 50: return __T("Text");
        default : return MI.Get(Stream_General, 0, General_FileExtension);
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_FileFormatCS_termID_MediaInfo(MediaInfo_Internal &MI)
{
    const Ztring &Format=MI.Get(Stream_General, 0, General_Format);

    if (Format==__T("MPEG-4"))
    {
        const Ztring &Format_Profile=MI.Get(Stream_General, 0, General_Format_Profile);
        int32u termID;
        if (Format_Profile==__T("QuickTime"))
            termID=160000;
        else
            termID=50000;
        Ztring CodecID=MI.Get(Stream_General, 0, General_CodecID);
        CodecID+=__T('/')+MI.Get(Stream_General, 0, General_CodecID_Compatible);
        if (false)
            ;
        else if (CodecID.find(__T("isom"))!=string::npos)
            termID+=100;
        else if (CodecID.find(__T("avc1"))!=string::npos)
            termID+=200;
        else if (CodecID.find(__T("iso2"))!=string::npos)
            termID+=300;
        else if (CodecID.find(__T("iso3"))!=string::npos)
            termID+=400;
        else if (CodecID.find(__T("iso4"))!=string::npos)
            termID+=500;
        else if (CodecID.find(__T("iso5"))!=string::npos)
            termID+=600;
        else if (CodecID.find(__T("iso6"))!=string::npos)
            termID+=700;
        else if (CodecID.find(__T("iso7"))!=string::npos)
            termID+=800;
        else if (CodecID.find(__T("iso8"))!=string::npos)
            termID+=900;
        else if (CodecID.find(__T("iso9"))!=string::npos)
            termID+=1000;
        else if (CodecID.find(__T("isoa"))!=string::npos)
            termID+=1100;
        else if (CodecID.find(__T("isob"))!=string::npos)
            termID+=1200;
        else if (CodecID.find(__T("isoc"))!=string::npos)
            termID+=1300;
        else if (CodecID.find(__T("mp41"))!=string::npos)
            termID+=100;
        else if (CodecID.find(__T("mp42"))!=string::npos)
            termID+=100;
        return termID;
    }
    if (Format==__T("MPEG Audio"))
    {
        if (MI.Get(Stream_Audio, 0, Audio_Format_Profile).find(__T('2'))!=string::npos)
            return 500000; //mp2
        if (MI.Get(Stream_Audio, 0, Audio_Format_Profile).find(__T('1'))!=string::npos)
            return 510000; //mp1
        return 0;
    }
    if (Format==__T("Wave") || Format==__T("Wave64"))
    {
        int32u termid;
        if (Format==__T("Wave64"))
            termid=530000;
        else if (MI.Get(Stream_General, 0, General_Format_Profile)==__T("RF64"))
            termid=520000; //Wav (RF64)
        else
            termid=90000;
        if (!MI.Get(Stream_General, 0, __T("bext_Present")).empty())
            termid+=100;
        Ztring Format_Settings=MI.Get(Stream_General, 0, General_Format_Settings);
        if (false)
            ;
        else if (Format_Settings.find(__T("WaveFormatExtensible"))!=string::npos)
            termid+=4;
        else if (Format_Settings.find(__T("WaveFormatEx"))!=string::npos)
            termid+=3;
        else if (Format_Settings.find(__T("PcmWaveformat"))!=string::npos)
            termid+=2;
        else if (Format_Settings.find(__T("WaveFormat"))!=string::npos)
            termid+=1;
        return termid;
    }
    if (Format==__T("DSF"))
        return 540000;
    if (Format==__T("DSDIFF"))
        return 550000;
    if (Format==__T("FLAC"))
        return 560000;
    if (Format==__T("AIFF"))
        return 570000;
    if (Format==__T("N19"))
        return 580000;
    if (Format==__T("SubRip"))
        return 590000;
    if (Format==__T("TTML"))
        return 600000;
    return 0;
}

//---------------------------------------------------------------------------
int32u Mpeg7_FileFormatCS_termID(MediaInfo_Internal &MI, size_t)
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
    if (Format==__T("Windows Media"))
        return 190000;
    if (Format==__T("ZIP"))
        return 100000;

    //Out of specs
    return Mpeg7_FileFormatCS_termID_MediaInfo(MI);
}

const char* Mpeg7_Wav_Extra_List[]=
{
    "WAVEFORMAT",
    "PCMWAVEFORMAT",
    "WAVEFORMATEX",
    "WAVEFORMATEXTENSIBLE",
};
size_t Mpeg7_Wav_Extra_List_Size=sizeof(Mpeg7_Wav_Extra_List)/sizeof(decltype(*Mpeg7_Wav_Extra_List));
static string Mpeg7_Wav_Extra(int32u Value)
{
    if (!Value || Value>Mpeg7_Wav_Extra_List_Size)
        return string();
    return string(1, ' ')+Mpeg7_Wav_Extra_List[Value-1];
}
Ztring Mpeg7_FileFormatCS_Name(int32u termID, MediaInfo_Internal &MI, size_t) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
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
        case  5 :   switch ((termID%10000)/100)
                    {
                        case  1 : return __T("mp4 isom");
                        case  2 : return __T("mp4 avc1");
                        case  3 : return __T("mp4 iso2");
                        case  4 : return __T("mp4 iso3");
                        case  5 : return __T("mp4 iso4");
                        case  6 : return __T("mp4 iso5");
                        case  7 : return __T("mp4 iso6");
                        case  8 : return __T("mp4 iso7");
                        case  9 : return __T("mp4 iso8");
                        case 10 : return __T("mp4 iso9");
                        case 11 : return __T("mp4 isoa");
                        case 12 : return __T("mp4 isob");
                        case 13 : return __T("mp4 isoc");
                        default : return __T("mp4");
                    }
        case  6 : return __T("dv");
        case  7 : return __T("avi");
        case  8 : return __T("bdf");
        case  9 :
                    {
                    const char* Core;
                    switch ((termID%10000)/100)
                    {
                        case 1 :    Core="bwf"; break;
                        default:    Core="wav"; break;
                    }
                    string Extra=Mpeg7_Wav_Extra(termID%100);
                    return Ztring().From_UTF8(Core+Extra);
                    }
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
        case 52 :
                    {
                    const char* Core;
                    switch ((termID%10000)/100)
                    {
                        case 1 :    Core="mbwf"; break;
                        default:    Core="wav-rf64"; break;
                    }
                    string Extra=Mpeg7_Wav_Extra(termID%100);
                    return Ztring().From_UTF8(Core+Extra);
                    }
        case 53 :
                    {
                    const char* Core="wave64";
                    string Extra=Mpeg7_Wav_Extra(termID%100);
                    return Ztring().From_UTF8(Core+Extra);
                    }
        case 54 : return __T("dsf");
        case 55 : return __T("dsdiff");
        case 56 : return __T("flac");
        case 57 : return __T("aiff");
        case 58 : return __T("stl");
        case 59 : return __T("srt");
        case 60 : return __T("ttml");
        default : return MI.Get(Stream_General, 0, General_Format);
    }
}

//---------------------------------------------------------------------------
extern size_t Avc_profile_level_Indexes(const string& ProfileLevelS);
extern size_t ProRes_Profile_Index(const string& ProfileS);
int32u Mpeg7_VisualCodingFormatCS_termID_MediaInfo(MediaInfo_Internal& MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Video, StreamPos, Video_Format);
    const Ztring &Profile=MI.Get(Stream_Video, StreamPos, Video_Format_Profile);

    if (Format==__T("AVC"))
    {
        auto ProfileLevelS=Profile.To_UTF8();
        auto ProfileLevel=Avc_profile_level_Indexes(ProfileLevelS);
        int32u ToReturn=500000;
        ToReturn+=(ProfileLevel>>8)*100; //Profile
        ToReturn+=ProfileLevel&0xFF; //Level
        return ToReturn;
    }
    if (Format==__T("HEVC"))
        return 510000;
    if (Format==__T("WMV"))
        return 520000;
    if (Format==__T("WMV2"))
        return 530000;
    if (Format==__T("ProRes"))
    {
        auto ProfileLevelS=Profile.To_UTF8();
        auto Profile=ProRes_Profile_Index(ProfileLevelS);
        int32u ToReturn=540000;
        ToReturn+=Profile*100; //Profile
        return ToReturn;
    }
    return 0;
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

    //Out of specs
    return Mpeg7_VisualCodingFormatCS_termID_MediaInfo(MI, StreamPos);
}

//---------------------------------------------------------------------------
Ztring Mpeg7_Visual_colorDomain(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &ChromaSubsampling=MI.Get(Stream_Video, StreamPos, Video_ChromaSubsampling);
    if (ChromaSubsampling.find(__T("4:"))!=string::npos)
        return __T("color");
    if (ChromaSubsampling==__T("Gray"))
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

Ztring Mpeg7_SystemCS_Name(int32u termID, MediaInfo_Internal &MI, size_t) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case  1 : return __T("PAL");
        case  2 : return __T("SECAM");
        case  3 : return __T("NTSC");
        default : return MI.Get(Stream_Video, 0, Video_Standard);
    }
}

//---------------------------------------------------------------------------
int32u Mpeg7_AudioCodingFormatCS_termID_MediaInfo(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Audio, StreamPos, Audio_Format);

    if (Format==__T("DSD"))
        return 500000;
    if (Format==__T("DST"))
        return 510000;
    if (Format==__T("FLAC"))
        return 520000;
    if (Format.find(__T("AAC"))==0)
        return 530000;
    if (Format==__T("WMA"))
        return 540000;
    return 0;
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

    //Out of specs
    return Mpeg7_AudioCodingFormatCS_termID_MediaInfo(MI, StreamPos);
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
        //Out of specs --> MediaInfo CS
        case 50 : return __T("DSD");
        case 51 : return __T("DST");
        case 52 : return __T("FLAC");
        case 53 : return __T("AAC");
        case 54 : return __T("WMA");
        default: return MI.Get(Stream_Audio, StreamPos, Audio_Format);
    }
}

//---------------------------------------------------------------------------
struct Mpeg7_AudioPresentationCS_Extra_Struct 
{
    size_t Index;
    int64u ChannelLayout;
    const char* Name;
};

enum ChannelIndex
{
    //Middle
    CI_M_000,
    CI_M_180,
    CI_M_L022,
    CI_M_R022,
    CI_M_L030,
    CI_M_R030,
    CI_M_L060,
    CI_M_R060,
    CI_M_L090,
    CI_M_R090,
    CI_M_L110,
    CI_M_R110,
    CI_M_L135,
    CI_M_R135,
    CI_M_LSCR,
    CI_M_RSCR,
    CI_M_LSD,
    CI_M_RSD,
    //Upper
    CI_U_000,
    CI_U_180,
    CI_U_L030,
    CI_U_R030,
    CI_U_L090,
    CI_U_R090,
    CI_U_L110,
    CI_U_R110,
    CI_U_L135,
    CI_U_R135,
    CI_T_000,
    //Lower
    CI_L_000,
    CI_L_L030,
    CI_L_R030,
    //LFE
    CI_LFE,
    CI_LFE2,
    CI_LFE3,
    //Misc
    CI_M_M,
    CI_M_M2,
    CI_M_LT,
    CI_M_RT,
    CI_MAX
};
const char* ChannelIndexMap[]=
{
    //Middle
    "C",
    "Cb",
    "Lc",
    "Rc",
    "L",
    "R",
    "Lw",
    "Rw",
    "Lss",
    "Rss",
    "Ls",
    "Rs",
    "Lb",
    "Rb",
    "Lscr",
    "Rscr",
    "Lsd",
    "Rsd",
    //Upper
    "Tfc",
    "Tbc",
    "Tfl",
    "Tfr",
    "Tsl",
    "Tsr",
    "Rvs",
    "Lvs",
    "Tbl",
    "Tbr",
    "Tc",
    //Lower
    "Bfc",
    "Bfl",
    "Bfr",
    //LFE
    "LFE",
    "LFE2",
    "LFE3",
    //Misc
    "M",
    "M",
    "Lt",
    "Rt",
};
static_assert(sizeof(ChannelIndexMap)/sizeof(decltype(*ChannelIndexMap))==CI_MAX, "");
enum ChannelMask : int64u
{
    //Middle
    MASK_M_000  = ((uint64_t)1)<<CI_M_000,
    MASK_M_180  = ((uint64_t)1)<<CI_M_180,
    MASK_M_L022 = ((uint64_t)1)<<CI_M_L022,
    MASK_M_R022 = ((uint64_t)1)<<CI_M_R022,
    MASK_M_022  = MASK_M_L022 | MASK_M_R022,
    MASK_M_L030 = ((uint64_t)1)<<CI_M_L030,
    MASK_M_R030 = ((uint64_t)1)<<CI_M_R030,
    MASK_M_030  = MASK_M_L030 | MASK_M_R030,
    MASK_M_L060 = ((uint64_t)1)<<CI_M_L060,
    MASK_M_R060 = ((uint64_t)1)<<CI_M_R060,
    MASK_M_060  = MASK_M_L060 | MASK_M_R060,
    MASK_M_L090 = ((uint64_t)1)<<CI_M_L090,
    MASK_M_R090 = ((uint64_t)1)<<CI_M_R090,
    MASK_M_090  = MASK_M_L090 | MASK_M_R090,
    MASK_M_L110 = ((uint64_t)1)<<CI_M_L110,
    MASK_M_R110 = ((uint64_t)1)<<CI_M_R110,
    MASK_M_110  = MASK_M_L110 | MASK_M_R110,
    MASK_M_L135 = ((uint64_t)1)<<CI_M_L135,
    MASK_M_R135 = ((uint64_t)1)<<CI_M_R135,
    MASK_M_135  = MASK_M_L135 | MASK_M_R135,
    MASK_M_LSCR = ((uint64_t)1)<<CI_M_LSCR,
    MASK_M_RSCR = ((uint64_t)1)<<CI_M_RSCR,
    MASK_M_SCR  = MASK_M_LSCR | MASK_M_RSCR,
    MASK_M_LSD  = ((uint64_t)1)<<CI_M_LSD,
    MASK_M_RSD  = ((uint64_t)1)<<CI_M_RSD,
    MASK_M_SD   = MASK_M_LSD | MASK_M_RSD,
    MASK_M_M    = ((uint64_t)1)<<CI_M_M,
    MASK_M_M2   = ((uint64_t)1)<<CI_M_M2,
    MASK_M_MM   = MASK_M_M    | MASK_M_M2,
    MASK_M_LT   = ((uint64_t)1)<<CI_M_LT,
    MASK_M_RT   = ((uint64_t)1)<<CI_M_RT,
    MASK_M_T    = MASK_M_LT   | MASK_M_RT,
    //Upper
    MASK_U_000  = ((uint64_t)1)<<CI_U_000,
    MASK_U_180  = ((uint64_t)1)<<CI_U_180,
    MASK_U_L030 = ((uint64_t)1)<<CI_U_L030,
    MASK_U_R030 = ((uint64_t)1)<<CI_U_R030,
    MASK_U_030  = MASK_U_L030 | MASK_U_R030,
    MASK_U_L090 = ((uint64_t)1)<<CI_U_L090,
    MASK_U_R090 = ((uint64_t)1)<<CI_U_R090,
    MASK_U_090  = MASK_U_L090 | MASK_U_R090,
    MASK_U_L110 = ((uint64_t)1)<<CI_U_L110,
    MASK_U_R110 = ((uint64_t)1)<<CI_U_R110,
    MASK_U_110  = MASK_U_L110 | MASK_U_R110,
    MASK_U_L135 = ((uint64_t)1)<<CI_U_L135,
    MASK_U_R135 = ((uint64_t)1)<<CI_U_R135,
    MASK_U_135  = MASK_U_L135 | MASK_U_R135,
    MASK_T_000  = ((uint64_t)1)<<CI_T_000,
    //Lower
    MASK_L_000  = ((uint64_t)1)<<CI_L_000,
    MASK_L_L030 = ((uint64_t)1)<<CI_L_L030,
    MASK_L_R030 = ((uint64_t)1)<<CI_L_R030,
    MASK_L_030  = MASK_L_L030 | MASK_L_R030,
    //LFE
    MASK_LFE    = ((uint64_t)1)<<CI_LFE,
    MASK_LFE2   = ((uint64_t)1)<<CI_LFE2,
    MASK_LFE3   = ((uint64_t)1)<<CI_LFE3,
};

#define ID(_A,_B) _A*100+_B
static const Mpeg7_AudioPresentationCS_Extra_Struct Mpeg7_AudioPresentationCS_Extra[] =
{
    // 1.0
    { ID(  2, 0), MASK_M_M    , "mono" },
    // 2.0
    { ID(  3, 0), MASK_M_030  , "stereo" },
    { ID(  3, 0), MASK_M_T    , "stereo" }, // stereo-matrix
    { ID(  3, 0), MASK_M_MM   , "stereo" }, // dual-mono
    // 2.1
    { ID( 50, 3), MASK_M_030 | MASK_LFE   , "2.1"},
    // 3.0
    { ID( 51, 1), MASK_M_030 | MASK_M_000 , "3.0 with 3 front speakers"},
    { ID( 51, 2), MASK_M_030 | MASK_M_180 , "3.0 with 2 front, 1 back speakers" },
    // 3.1
    { ID( 52, 1), MASK_M_030 | MASK_M_000 | MASK_LFE   , "3.1 with 3 front speakers"},
    { ID( 52, 2), MASK_M_030 | MASK_M_180 | MASK_LFE   , "3.1 with 2 front, 1 back speakers" },
    // 4.0
    { ID( 54, 1), MASK_M_030 | MASK_M_110              , "4.0 with 2 front, 2 surround speakers" },
    { ID( 54, 2), MASK_M_030 | MASK_M_135              , "4.0 with 2 front, 2 back speakers"},
    { ID( 54, 3), MASK_M_030 | MASK_M_000 | MASK_M_180 , "4.0 with 3 front, 1 back speakers" },
    // 4.1
    { ID( 54, 1), MASK_M_030 | MASK_M_110              | MASK_LFE    , "4.1 with 2 front, 2 surround speakers" },
    { ID( 54, 2), MASK_M_030 | MASK_M_135              | MASK_LFE    , "4.1 with 2 front, 2 back speakers"},
    { ID( 54, 3), MASK_M_030 | MASK_M_000 | MASK_M_180 | MASK_LFE    , "4.1 with 3 front, 1 back speakers" },
    // 5.0
    { ID( 55, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 , "5.0 with 3 front, 2 side speakers" },
    { ID( 55, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 , "5.0 with 3 front, 2 surround speakers" },
    { ID( 55, 3), MASK_M_030 | MASK_M_000 | MASK_M_135 , "5.0 with 3 front, 2 back speakers" },
    // 5.1
    { ID(  5, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_LFE   , "Home theater 5.1 with 3 front, 2 side speakers" },
    { ID(  5, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_LFE   , "Home theater 5.1 with 3 front, 2 surround speakers" },
    { ID(  5, 3), MASK_M_030 | MASK_M_000 | MASK_M_135 | MASK_LFE   , "Home theater 5.1 with 3 front, 2 back speakers" },
    // 6.0
    { ID( 56, 4), MASK_M_030 | MASK_M_090 | MASK_M_135              , "6.0 with 2 front, 2 side, 2 back speakers" },
    { ID( 56, 5), MASK_M_030 | MASK_M_110 | MASK_M_135              , "6.0 with 2 front, 2 surround, 2 back speakers" },
    { ID( 56, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_180 , "6.0 with 3 front, 2 side, 1 back speakers" },
    { ID( 56, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_180 , "6.0 with 3 front, 2 surround, 1 back speakers" },
    { ID( 56, 3), MASK_M_030 | MASK_M_000 | MASK_M_135 | MASK_M_180 , "6.0 with 3 front, 3 back speakers" },
    { ID( 56, 6), MASK_M_030 | MASK_M_022 | MASK_M_090              , "6.0 with 4 front, 2 side speakers" },
    { ID( 56, 7), MASK_M_030 | MASK_M_022 | MASK_M_110              , "6.0 with 4 front, 2 surround speakers" },
    { ID( 56, 8), MASK_M_030 | MASK_M_022 | MASK_M_135              , "6.0 with 4 front, 2 back speakers" },
    // 6.1
    { ID( 57, 4), MASK_M_030 | MASK_M_090 | MASK_M_135              | MASK_LFE   , "6.1 with 2 front, 2 side, 2 back speakers" },
    { ID( 57, 5), MASK_M_030 | MASK_M_110 | MASK_M_135              | MASK_LFE   , "6.1 with 2 front, 2 surround, 2 back speakers" },
    { ID( 57, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_180 | MASK_LFE   , "6.1 with 3 front, 2 side, 1 back speakers" },
    { ID( 57, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_180 | MASK_LFE   , "6.1 with 3 front, 2 surround, 1 back speakers" },
    { ID( 57, 3), MASK_M_030 | MASK_M_000 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "6.1 with 3 front, 3 back speakers" },
    { ID( 57, 6), MASK_M_030 | MASK_M_022 | MASK_M_090              | MASK_LFE   , "6.1 with 4 front, 2 side speakers" },
    { ID( 57, 7), MASK_M_030 | MASK_M_022 | MASK_M_110              | MASK_LFE   , "6.1 with 4 front, 2 surround speakers" },
    { ID( 57, 8), MASK_M_030 | MASK_M_022 | MASK_M_135              | MASK_LFE   , "6.1 with 4 front, 2 back speakers" },
    // 6.2
    { ID( 58, 4), MASK_M_030 | MASK_M_090 | MASK_M_135              | MASK_LFE   | MASK_LFE2   , "6.2 with 2 front, 2 side, 2 back speakers" },
    { ID( 58, 5), MASK_M_030 | MASK_M_110 | MASK_M_135              | MASK_LFE   | MASK_LFE2   , "6.2 with 2 front, 2 surround, 2 back speakers" },
    { ID( 58, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_180 | MASK_LFE   | MASK_LFE2   , "6.2 with 3 front, 2 side, 1 back speakers" },
    { ID( 58, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_180 | MASK_LFE   | MASK_LFE2   , "6.2 with 3 front, 2 surround, 1 back speakers" },
    { ID( 58, 3), MASK_M_030 | MASK_M_000 | MASK_M_135 | MASK_M_180 | MASK_LFE   | MASK_LFE2   , "6.2 with 3 front, 3 back speakers" },
    { ID( 58, 6), MASK_M_030 | MASK_M_022 | MASK_M_090              | MASK_LFE   | MASK_LFE2   , "6.2 with 4 front, 2 side speakers" },
    { ID( 58, 7), MASK_M_030 | MASK_M_022 | MASK_M_110              | MASK_LFE   | MASK_LFE2   , "6.2 with 4 front, 2 surround speakers" },
    { ID( 58, 8), MASK_M_030 | MASK_M_022 | MASK_M_135              | MASK_LFE   | MASK_LFE2   , "6.2 with 4 front, 2 back speakers" },
    // 7.0
    { ID( 59, 3), MASK_M_030 | MASK_M_090 | MASK_M_135 | MASK_M_180 , "7.0 with 2 front, 2 side, 3 back speakers" },
    { ID( 59, 4), MASK_M_030 | MASK_M_110 | MASK_M_135 | MASK_M_180 , "7.0 with 2 front, 2 surround, 3 back speakers" },
    { ID( 59, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 , "7.0 with 3 front, 2 side, 2 back speakers" },
    { ID( 59, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 , "7.0 with 3 front, 2 surround, 2 back speakers" },
    { ID( 59, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_090 , "7.0 with 5 front, 2 side speakers" },
    { ID( 59, 6), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 , "7.0 with 5 front, 2 surround speakers" },
    { ID( 59, 7), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 , "7.0 with 5 front, 2 back speakers" },
    // 7.1
    { ID(  6, 3), MASK_M_030 | MASK_M_090 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "Movie theater with 2 front, 2 side, 3 back speakers" },
    { ID(  6, 4), MASK_M_030 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "Movie theater with 2 front, 2 surround, 3 back speakers" },
    { ID(  6, 1), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_110 | MASK_LFE   , "Movie theater with 3 front, 2 wide, 2 surround speakers" },
    { ID(  6, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_LFE   , "Movie theater with 3 front, 2 side, 2 back speakers" },
    { ID(  6, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_LFE   , "Movie theater with 3 front, 2 surround, 2 back speakers" },
    { ID(  6, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_090 | MASK_LFE   , "Movie theater with 5 front, 2 side speakers" },
    { ID(  6, 6), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_LFE   , "Movie theater with 5 front, 2 surround speakers" },
    { ID(  6, 7), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 | MASK_LFE   , "Movie theater with 5 front, 2 back speakers" },
    // 7.2
    { ID( 60, 3), MASK_M_030 | MASK_M_090 | MASK_M_135 | MASK_M_180 | MASK_LFE   | MASK_LFE2   , "7.2 with 2 front, 2 side, 3 back speakers" },
    { ID( 60, 4), MASK_M_030 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_LFE   | MASK_LFE2   , "7.2 with 2 front, 2 surround, 3 back speakers" },
    { ID( 60, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_LFE   | MASK_LFE2   , "7.2 with 3 front, 2 side, 2 back speakers" },
    { ID( 60, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_LFE   | MASK_LFE2   , "7.2 with 3 front, 2 surround, 2 back speakers" },
    { ID( 60, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_LFE   | MASK_LFE2   , "7.2 with 5 front, 2 side speakers" },
    { ID( 60, 6), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_LFE   | MASK_LFE2   , "7.2 with 5 front, 2 surround speakers" },
    { ID( 60, 7), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 | MASK_LFE   | MASK_LFE2   , "7.2 with 5 front, 2 back speakers" },
    // 8.0
    { ID( 61, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_M_180 , "8.0 with 3 front, 2 side, 3 back speakers" },
    { ID( 61, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_M_180 , "8.0 with 3 front, 2 surround, 3 back speakers" },
    { ID( 61, 3), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_090 | MASK_M_180 , "8.0 with 5 front, 2 side, 1 back speakers" },
    { ID( 61, 4), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_M_180 , "8.0 with 5 front, 2 surround, 1 back speakers" },
    { ID( 61, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 | MASK_M_180 , "8.0 with 5 front, 3 back speakers" },
    // 8.1
    { ID( 62, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "8.1 with 3 front, 2 side, 3 back speakers" },
    { ID( 62, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "8.1 with 3 front, 2 surround, 3 back speakers" },
    { ID( 62, 3), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_M_180 | MASK_LFE   , "8.1 with 5 front, 2 side, 1 back speakers" },
    { ID( 62, 4), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_M_180 | MASK_LFE   , "8.1 with 5 front, 2 surround, 1 back speakers" },
    { ID( 62, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 | MASK_M_180 | MASK_LFE   , "8.1 with 5 front, 3 back speakers" },
    // 8.2
    { ID( 63, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_LFE | MASK_LFE2   , "8.2 with 3 front, 2 side, 3 back speakers" },
    { ID( 63, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_LFE | MASK_LFE2   , "8.2 with 3 front, 2 surround, 3 back speakers" },
    { ID( 63, 3), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_LFE | MASK_LFE2   , "8.2 with 5 front, 2 side, 1 back speakers" },
    { ID( 63, 4), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_LFE | MASK_LFE2   , "8.2 with 5 front, 2 surround, 1 back speakers" },
    { ID( 63, 5), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_135 | MASK_LFE | MASK_LFE2   , "8.2 with 5 front, 3 back speakers" },
    // 22.2 (10.2.9.3)
    { ID(100, 0), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_090 | MASK_M_135 | MASK_M_180 | MASK_U_030 | MASK_U_000 | MASK_U_090 | MASK_T_000 | MASK_U_135 | MASK_U_180 | MASK_L_030 | MASK_L_000 | MASK_LFE   | MASK_LFE2  , "Hamasaki 22.2" },
    { ID(100, 0), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_U_030 | MASK_U_000 | MASK_U_090 | MASK_T_000 | MASK_U_135 | MASK_U_180 | MASK_L_030 | MASK_L_000 | MASK_LFE   | MASK_LFE2  , "Hamasaki 22.2" },
    // 2.0.2
    { ID(101, 1), MASK_M_030 | MASK_U_030 , "2.0.2 with 2 front, 2 topfront speakers" },
    // 2.1.2
    { ID(102, 1), MASK_M_030 | MASK_U_030 | MASK_LFE   , "2.1.2 with 2 front, 2 topfront speakers" },
    // 3.0.2
    { ID(103, 1), MASK_M_030 | MASK_M_000 | MASK_U_030 , "3.0.2 with 3 front, 2 topfront speakers" },
    // 3.1.2
    { ID(104, 1), MASK_M_030 | MASK_M_000 | MASK_U_030 | MASK_LFE   , "3.1.2 with 3 front, 2 topfront speakers" },
    // 5.1.2
    { ID(105, 1), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_030 | MASK_LFE   , "5.1.2 with 3 front, 2 surround, 2 topfront speakers" },
    { ID(105, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_090 | MASK_LFE   , "5.1.2 with 3 front, 2 surround, 2 topside speakers" },
    { ID(105, 3), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_110 | MASK_LFE   , "5.1.2 with 3 front, 2 surround, 2 topsurround speakers" },
    { ID(105, 4), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_135 | MASK_LFE   , "5.1.2 with 3 front, 2 surround, 2 topback speakers" },
    // 5.1.4
    { ID(106, 1), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_030 | MASK_U_090 | MASK_LFE   , "5.1.4 with 3 front, 2 surround, 2 topfront, 2 topside speakers" },
    { ID(106, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_030 | MASK_U_110 | MASK_LFE   , "5.1.4 with 3 front, 2 surround, 2 topfront, 2 topsurround speakers" },
    { ID(106, 3), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_030 | MASK_U_135 | MASK_LFE   , "5.1.4 with 3 front, 2 surround, 2 topfront, 2 topback speakers" },
    // 5.1.6
    { ID(107, 1), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_U_030 | MASK_U_000 | MASK_U_110 | MASK_T_000 | MASK_LFE   , "5.1.6 with 3 front, 2 surround, 3 topfront, 1 top, 2 topsurround speakers" },
    // 7.1.4
    { ID(108, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_U_030 | MASK_U_135 | MASK_LFE   , "7.1.4 with 3 front, 2 side, 2 back, 2 topfront, 2 topback speakers" },
    { ID(108, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_135 | MASK_LFE   , "7.1.4 with 3 front, 2 surround, 2 back, 2 topfront, 2 topback speakers" },
    // 7.2.3
    { ID(109, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_180 | MASK_U_030 | MASK_T_000 | MASK_LFE   | MASK_LFE2  , "7.2.3 with 3 front, 2 side, 2 back, 2 topfront, 1 top speakers" },
    { ID(109, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_180 | MASK_U_030 | MASK_T_000 | MASK_LFE   | MASK_LFE2  , "7.2.3 with 3 front, 2 surround, 2 back, 2 topfront, 1 top speakers" },
    // 7.1.6
    { ID(110, 1), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_U_030 | MASK_U_000 | MASK_T_000 | MASK_U_090 | MASK_LFE   , "7.1.6 with 3 front, 2 side, 2 back, 3 topfront, 1 top, 2 topside speakers" },
    { ID(110, 2), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_000 | MASK_T_000 | MASK_U_110 | MASK_LFE   , "7.1.6 with 3 front, 2 surround, 2 back, 3 topfront, 1 top, 2 topsurround speakers" },
    { ID(110, 3), MASK_M_030 | MASK_M_000 | MASK_M_090 | MASK_M_135 | MASK_U_030 | MASK_U_090 | MASK_U_135 | MASK_LFE   , "7.1.6 with 3 front, 2 side, 2 back, 2 topfront, 2 topside, 2 topback speakers" },
    { ID(110, 4), MASK_M_030 | MASK_M_000 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_110 | MASK_U_135 | MASK_LFE   , "7.1.6 with 3 front, 2 surround, 2 back, 2 topfront, 2 topsurround, 2 topback speakers" },
    // 9.1.4
    { ID(111, 1), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_090 | MASK_M_135 | MASK_U_030 | MASK_U_135 | MASK_LFE   , "9.1.6 with 5 front, 2 side, 2 back, 2 topfront, 2 topback speakers" },
    { ID(111, 2), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_135 | MASK_LFE   , "9.1.6 with 5 front, 2 surround, 2 back, 2 topfront, 2 topback speakers" },
    // 9.1.6
    { ID(112, 1), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_090 | MASK_M_135 | MASK_U_030 | MASK_U_090 | MASK_U_135 | MASK_LFE   , "9.1.6 with 5 front, 2 side, 2 back, 2 topfront, 2 topside, 2 topback speakers" },
    { ID(112, 2), MASK_M_030 | MASK_M_000 | MASK_M_022 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_110 | MASK_U_135 | MASK_LFE   , "9.1.6 with 5 front, 2 surround, 2 back, 2 topfront, 2 topsurround, 2 topback speakers" },
    { ID(112, 3), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_110 | MASK_M_135 | MASK_U_030 | MASK_U_090 | MASK_U_135 | MASK_LFE   , "9.1.6 with 3 front, 2 side, 2 surround, 2 back, 2 topfront, 2 topside, 2 topback speakers" },
    // 10.0.6
    { ID(113, 1), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_U_030 | MASK_U_090 | MASK_U_135 , "10.0.6 with 3 front, 2 wide, 2 surround, 3 back, 2 topfront, 2 topside, 2 topback speakers" },
    { ID(113, 2), MASK_M_030 | MASK_M_000 | MASK_M_060 | MASK_M_110 | MASK_M_135 | MASK_M_180 | MASK_U_030 | MASK_U_000 | MASK_U_135 | MASK_U_180 , "10.0.6 with 3 front, 2 wide, 2 surround, 3 back, 3 topfront, 3 topback speakers" },
};

int32u Mpeg7_AudioPresentationCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    //Get ChannelLayout and remove disabled channels e.g. from Dolby E
    ZtringList ChannelLayout;
    ChannelLayout.Separator_Set(0, __T(" "));
    Ztring ChannelLayoutZ=MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout);
    if (ChannelLayoutZ.empty())
        ChannelLayoutZ=MI.Get(Stream_Audio, StreamPos, __T("Substream0 ChannelLayout"));
    ChannelLayout.Write(ChannelLayoutZ);
    int64u ChannelLayoutI=0;
    for (const auto& Item : ChannelLayout)
    {
        string ItemS=Item.To_UTF8();
        if (ItemS=="X")
            continue; //Ignore masked channels
        if (ItemS=="M" && (ChannelLayoutI&MASK_M_M))
        {
            ChannelLayoutI|=MASK_M_M2;
            continue;
        }
        if (ChannelLayout.size()==1 && ItemS=="C" && MI.Count_Get(Stream_Audio)==1)
        {
            ChannelLayoutI|=MASK_M_M;
            continue;
        }
        if (ItemS=="Lscr")
            ItemS="Lc";
        if (ItemS=="Rscr")
            ItemS="Rc";
        size_t i=0;
        for (; i<CI_MAX; i++)
            if (ChannelIndexMap[i]==ItemS)
            {
                ChannelLayoutI|=((uint64_t)1)<<i;
                break;
            }
        if (i==CI_MAX)
            return 0;
    }

    if (std::bitset<64>(ChannelLayoutI).count()!=ChannelLayout.size())
        return 0;

    //Not ins specs
    for (auto Pos=begin(Mpeg7_AudioPresentationCS_Extra); Pos!=end(Mpeg7_AudioPresentationCS_Extra); ++Pos)
    {
        if (Pos->ChannelLayout==ChannelLayoutI)
            return Pos->Index*100;
    }

    const auto Channels=MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_).To_int32u();
    switch (Channels)
    {
        case 1: return 20000;
        case 2: return 30000;
    }
    return 0;
}

Ztring Mpeg7_AudioPresentationCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos)
{
    int32u termID0=termID/10000;
    int32u termID1=termID%10000;
    termID1/=100;
    termID/=100;
    if (!termID1)
    {
        switch (termID0)
        {
            case   2:
            case   3:
            case 100:
                break;
            default:
                termID+=1; //Using the first available then trim
        }
    }
    for (const auto& Mpeg7_AudioPresentationCS_Extra_Item : Mpeg7_AudioPresentationCS_Extra)
    {
        if (Mpeg7_AudioPresentationCS_Extra_Item.Index==termID)
        {
            Ztring Result;
            Result.From_UTF8(Mpeg7_AudioPresentationCS_Extra_Item.Name);
            if (!termID1)
            {
                auto With=Result.find(__T(" with"));
                if (With!=string::npos)
                    Result.resize(With);
            }
            return Result;
        }
    }

    return MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout);
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
int32u Mpeg7_TextualCodingFormatCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Text, StreamPos, Text_Format);

    if (Format==__T("N19"))
        return 500000;
    if (Format==__T("EIA-608"))
        return 510000;
    if (Format==__T("EIA-708"))
        return 520000;
    if (Format==__T("SubRip"))
        return 530000;
    if (Format==__T("Timed Text"))
        return 540000;
    if (Format==__T("TTML"))
        return 550000;

    return 0;
}

Ztring Mpeg7_TextualCodingFormatCS_Name(int32u termID, MediaInfo_Internal& MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case  50 : return __T("STL");
        case  53 : return __T("SRT");
        case  54 : return __T("MPEG-4 Part 17 Timed Text");
        case  55 : return __T("TTML");
        default  : return MI.Get(Stream_Text, StreamPos, Text_Format);
    }
}

//---------------------------------------------------------------------------
string Mpeg7_CreateTime(int64u Count, int64u Rate, bool IsP=false)
{
    int64u DD=Count/(24*60*60*Rate);
    Count=Count%(24*60*60*Rate);
    int64u HH=Count/(60*60*Rate);
    Count=Count%(60*60*Rate);
    int64u MM=Count/(60*Rate);
    Count=Count%(60*Rate);
    int64u SS=Count/Rate;
    Count=Count%Rate;
    string ToReturn;
    if (IsP)
        ToReturn+='P';
    if (DD)
    {
        ToReturn+=to_string(DD);
        ToReturn+='D';
    }
    ToReturn+='T';
    if (!IsP && HH<10)
        ToReturn+='0';
    ToReturn+=to_string(HH);
    ToReturn+=IsP?'H':':';
    if (!IsP && MM<10)
        ToReturn+='0';
    ToReturn+=to_string(MM);
    ToReturn+=IsP?'M':':';
    if (!IsP && SS<10)
        ToReturn+='0';
    ToReturn+=to_string(SS);
    ToReturn+=IsP?'S':':';
    if (Rate>1)
    {
        ToReturn+=to_string(Count);
        ToReturn+=IsP?'N':'F';
        ToReturn+=to_string(Rate);
        if (IsP)
            ToReturn+='F';
    }
    return ToReturn;
}

//---------------------------------------------------------------------------
string Mpeg7_MediaTimePoint(MediaInfo_Internal &MI, size_t Menu_Pos=(size_t)-1)
{
    int64u Count, Rate;
    if (Menu_Pos!=(size_t)-1)
    {
        auto CountS=MI.Get(Stream_Menu, Menu_Pos, Menu_Delay);
        if (CountS.empty())
            return {};
        Count=CountS.To_int64u();
        Rate=1000;
    }
    else
    {
        if (MI.Count_Get(Stream_Video)==1 && MI.Get(Stream_General, 0, General_Format)==__T("MPEG-PS"))
        {
            auto DelayS=MI.Get(Stream_Video, 0, Video_Delay);
            if (DelayS.empty())
                return {};
            auto Delay=DelayS.To_float64();
            Count=(int64u)float64_int64s(Delay*90);
            Rate=90000;
        }
        else if (MI.Count_Get(Stream_Audio)==1 && MI.Get(Stream_General, 0, General_Format)==__T("Wave"))
        {
            auto DelayS=MI.Get(Stream_Audio, 0, Audio_Delay);
            if (DelayS.empty())
                return {};
            Rate=MI.Get(Stream_Audio, 0, Audio_SamplingRate).To_int64u();
            if (Rate)
            {
                auto Delay=DelayS.To_float64();
                Count=(int64u)float64_int64s(Delay*Rate/1000);
            }
            else
            {
                Count=DelayS.To_int64u();
                Rate=1000;
            }
        }
        else
        {
            //Default: In milliseconds
            auto CountS=MI.Get(Stream_Video, 0, Video_Delay);
            if (CountS.empty())
                return {};
            Count=CountS.To_int64u();
            Rate=1000;
        }
    }
    return Mpeg7_CreateTime(Count, Rate);
}

//---------------------------------------------------------------------------
string Mpeg7_MediaDuration(MediaInfo_Internal &MI, size_t Menu_Pos=(size_t)-1)
{
    int64u Count, Rate;
    if (Menu_Pos!=(size_t)-1)
    {
        Count=MI.Get(Stream_Menu, Menu_Pos, Menu_FrameCount).To_int64u();
        Rate=(int64u)float64_int64s(MI.Get(Stream_Menu, Menu_Pos, Menu_FrameRate).To_int64u());
        if (!Count || !Rate)
        {
            Count=MI.Get(Stream_Menu, Menu_Pos, Menu_Duration).To_int64u();
            Rate=1000;
        }
    }
    else
    {
        if (MI.Count_Get(Stream_Video)==1)
        {
            Count=MI.Get(Stream_Video, 0, Video_FrameCount).To_int64u();
            Rate=(int64u)float64_int64s(MI.Get(Stream_Video, 0, Video_FrameRate).To_int64u());
        }
        else if (MI.Count_Get(Stream_Audio)==1)
        {
            Count=MI.Get(Stream_Audio, 0, Audio_SamplingCount).To_int64u();
            Rate=(int64u)float64_int64s(MI.Get(Stream_Audio, 0, Audio_SamplingRate).To_int64u());
        }
        else
        {
            //Default: In milliseconds
            Count=MI.Get(Stream_General, 0, General_Duration).To_int64u();
            Rate=1000;
        }
    }
    if (!Count || !Rate)
        return {};
    return Mpeg7_CreateTime(Count, Rate, true);
}

//---------------------------------------------------------------------------
Ztring Mpeg7_StripExtraValues(Ztring Value)
{
    if (Value.empty())
        return Value;

    size_t SlashPos=Value.find(__T(" / "));
    if (SlashPos!=string::npos)
        Value.erase(SlashPos);

    return Value;
}

//---------------------------------------------------------------------------
static Ztring Mpeg7_termID2String(int32u termID, const char* CS)
{
    Ztring ToReturn;

    //Unknown
    if (!termID)
    {
        ToReturn="urn:x-mpeg7-mediainfo:cs:";
        ToReturn+=Ztring(CS);
        ToReturn+=Ztring(":2009:unknown");
        return ToReturn;
    }

    //URN prefix
    bool OutOfSpec=(termID>=500000);
    if (OutOfSpec)
        ToReturn="urn:x-mpeg7-mediainfo:cs:";
    else
        ToReturn="urn:mpeg:mpeg7:cs:";
    ToReturn+=Ztring(CS);
    if (OutOfSpec)
        ToReturn+=Ztring(":2009:");
    else
        ToReturn+=Ztring(":2001:");

    //URN value
    ToReturn+=Ztring::ToZtring(termID/10000);
    if (termID%10000)
    {
        ToReturn+=__T('.');
        ToReturn+=Ztring::ToZtring((termID%10000)/100);
        if (termID%100)
        {
            ToReturn+=__T('.');
            ToReturn+=Ztring::ToZtring(termID%100);
        }
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
typedef int32u(*Mpeg7_termID) (MediaInfo_Internal &MI, size_t StreamPos);
typedef Ztring(*Mpeg7_Name) (int32u termID, MediaInfo_Internal &MI, size_t StreamPos);
static Node* Mpeg7_CS(Node* Node_MediaFormat, const char* MainName, const char* CS, const Mpeg7_termID& termIDFunction, const Mpeg7_Name& NameFunction, MediaInfo_Internal &MI, size_t StreamPos, bool Mandatory=false, bool FullDigits=false)
{
    int32u termID=termIDFunction(MI, StreamPos);
    Ztring Value=NameFunction(FullDigits?termID:((termID/10000)*10000), MI, StreamPos);
    if (!Mandatory && Value.empty())
        return NULL;

    Node* Node_Main=Node_MediaFormat->Add_Child(MainName);
    Node_Main->Add_Attribute("href", Mpeg7_termID2String(FullDigits?termID:((termID/10000)*10000), CS));

    Node_Main->Add_Child("mpeg7:Name", Value, "xml:lang", "en");
    if (!FullDigits && termID%10000)
    {
        Node* Node_Term=Node_Main->Add_Child("mpeg7:Term");
        Node_Term->Add_Attribute("termID", Ztring::ToZtring(termID/10000)+__T(".")+
                                            Ztring::ToZtring((termID%10000)/100));

        Value=NameFunction((termID/100)*100, MI, StreamPos);
        Node_Term->Add_Child("mpeg7:Name", Value, "xml:lang", "en");
        if (termID%100)
        {
            Node_Term=Node_Term->Add_Child("mpeg7:Term");
            Node_Term->Add_Attribute("termID", Ztring::ToZtring(termID/10000)+__T(".")+
                                                Ztring::ToZtring((termID%10000)/100)+__T(".")+
                                                Ztring::ToZtring(termID%100));

            Value=NameFunction(termID, MI, StreamPos);
            if (!Value.empty())
                Node_Term->Add_Child("mpeg7:Name", Value, "xml:lang", "en");
        }
    }

    return Node_Main;
}

//---------------------------------------------------------------------------
static const char* Mpeg7_ServiceKind2Type[][2] =
{
    { "Dubbed", "dubbed"},
    { "Commentary", "commentary"},
    { "EasyReader", "easyReader"},
    { "HI", "hearingImpaired"},
    { "HI-ME", "hearingImpaired"},
    { "HI-D", "hearingImpaired"},
    { "Translated", "translated"},
    { "Supplementary", "supplementary"},
    { "VI", "visuallyImpaired"},
    { "VI-ME", "visuallyImpaired"},
    { "VI-D", "visuallyImpaired"},
};
static const size_t Mpeg7_ServiceKind2Type_Size=sizeof(Mpeg7_ServiceKind2Type)/sizeof(*Mpeg7_ServiceKind2Type);
static const char* Mpeg7_ServiceKind2Type_Get(const char* Value)
{

    for (size_t i=0; i<Mpeg7_ServiceKind2Type_Size; i++)
        if (!strcmp(Value, Mpeg7_ServiceKind2Type[i][0]))
            return Mpeg7_ServiceKind2Type[i][1];
    return nullptr;
}


//---------------------------------------------------------------------------
void Mpeg7_Create_IdRef(Node* N, bool IsRef, const char* Name, size_t Ref, size_t Menu_Pos=(size_t)-1)
{
    string Result(Name);
    Result+='.';
    Result+=to_string(Ref+1);
    if (Menu_Pos!=(size_t)-1)
    {
        Result+='.';
        Result+=to_string(Menu_Pos+1);
    }
    N->Add_Attribute(IsRef?"ref":"id", Result);
}
inline void Mpeg7_Create_Id (Node* N, const char* Name, size_t Ref, size_t Menu_Pos=(size_t)-1) { Mpeg7_Create_IdRef(N, false, Name, Ref, Menu_Pos); }
inline void Mpeg7_Create_Ref(Node* N, const char* Name, size_t Ref, size_t Menu_Pos=(size_t)-1) { Mpeg7_Create_IdRef(N, true , Name, Ref, Menu_Pos); }

//---------------------------------------------------------------------------
void Mpeg7_Create_StreamID(Node* N, bool Extended, MediaInfo_Internal& MI, stream_t StreamKind, size_t StreamPos)
{
    Ztring StreamID=MI.Get(StreamKind, StreamPos, General_ID);
    if (StreamID.empty())
        return;
    Ztring Main, Sub;
    auto Pos=StreamID.find(__T('-'));
    if (Pos!=(size_t)-1)
    {
        Main=StreamID.substr(0, Pos);
        Sub=StreamID.substr(Pos+1);
        auto Pos2=Sub.find(__T('-'));
        if (Pos!=(size_t)-1)
            Sub.resize(Pos); // Removing other subIDs
    }
    else
        Main=StreamID;

    if (Extended || StreamPos || StreamKind==Stream_Text)
    {
        auto MediaLocator=N->Add_Child("mpeg7:MediaLocator");
        Ztring Source=MI.Get(StreamKind, StreamPos, __T("Source"));
        if (!Source.empty())
        {
            #ifdef WINDOWS
            Source.FindAndReplace(__T("\\"), __T("/"), Ztring_Recursive);
            #endif
            MediaLocator->Add_Child("mpeg7:MediaUri", Source);
        }
        MediaLocator->Add_Child("mpeg7:StreamID", Main);
        if (!Sub.empty())
            MediaLocator->Add_Child("mpeg7:SubstreamID", Sub);
    }
    else
    {
        N->Add_Child("")->XmlCommentOut="StreamID: "+Main.To_UTF8();
        if (!Sub.empty())
            N->Add_Child("")->XmlCommentOut="SubstreamID : "+Sub.To_UTF8();
    }
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
void Mpeg7_Transform_Visual(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, size_t Extended, size_t Menu_Pos=(size_t)-1)
{
    Node* Node_VisualCoding=Parent->Add_Child("mpeg7:VisualCoding");

    //ID
    if (Extended)
        Mpeg7_Create_Id(Node_VisualCoding, "visual", StreamPos, Menu_Pos);

    //StreamID
    Mpeg7_Create_StreamID(Node_VisualCoding, Extended, MI, Stream_Video, StreamPos);

    //Format
    Node* Node_Format=Mpeg7_CS(Node_VisualCoding, "mpeg7:Format", "VisualCodingFormatCS", Mpeg7_VisualCodingFormatCS_termID, VideoCompressionCodeCS_Name, MI, StreamPos);
    if (Node_Format)
    {
    Ztring Value=Mpeg7_Visual_colorDomain(MI, StreamPos); //Algo puts empty string if not known
    if (!Value.empty())
        Node_Format->Add_Attribute("colorDomain", Value);
    }

    //Pixel
    if (!MI.Get(Stream_Video, StreamPos, Video_PixelAspectRatio).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_BitDepth).empty())
    {
        Node* Node_Pixel=Node_VisualCoding->Add_Child("mpeg7:Pixel");
        Node_Pixel->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_PixelAspectRatio, "aspectRatio");
        Ztring bitsPer=Mpeg7_StripExtraValues(MI.Get(Stream_Video, StreamPos, Video_BitDepth));
        if (!bitsPer.empty())
            Node_Pixel->Add_Attribute("bitsPer", bitsPer);
    }

    //Frame
    if (!MI.Get(Stream_Video, StreamPos, Video_DisplayAspectRatio).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_Height).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_Width).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_FrameRate).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_FrameRate_Mode).empty()
     || !MI.Get(Stream_Video, StreamPos, Video_ScanType).empty())
    {
        Node* Node_Frame=Node_VisualCoding->Add_Child("mpeg7:Frame");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_DisplayAspectRatio, "aspectRatio");

        Ztring Height=Mpeg7_StripExtraValues(MI.Get(Stream_Video, StreamPos, Video_Height));
        if (!Height.empty())
            Node_Frame->Add_Attribute("height", Height);

        Ztring Width=Mpeg7_StripExtraValues(MI.Get(Stream_Video, StreamPos, Video_Width));
        if (!Width.empty())
            Node_Frame->Add_Attribute("width", Width);

        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_FrameRate, "rate");

        Ztring Value=MI.Get(Stream_Video, StreamPos, Video_ScanType).MakeLowerCase();
        if (!Value.empty())
        {
            if (Value==__T("mbaff") || Value==__T("interlaced"))
                Node_Frame->Add_Attribute("structure", "interlaced");
            else if (Value==__T("progressive"))
                Node_Frame->Add_Attribute("structure", "progressive");
        }

        if (MI.Get(Stream_Video, StreamPos, Video_FrameRate_Mode)==__T("VFR"))
        {
            if (Extended)
                Node_Frame->Add_Attribute("variableRate", "true");
            else
                Node_Frame->XmlComment="variableRate: true";
        }
    }

    //ChromaSubsampling
    if (MI.Get(Stream_Video, StreamPos, Video_ChromaSubsampling).find(__T("4:2:0"))!=string::npos)
    {
        Node* Node_ColorSampling=Node_VisualCoding->Add_Child("mpeg7:ColorSampling");
        if (Extended)
            Node_ColorSampling->Add_Child("mpeg7:Name", std::string("YUV 4:2:0 Interlaced"));
        else
            Node_ColorSampling->XmlComment="YUV 4:2:0 Interlaced";
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
    if (MI.Get(Stream_Video, StreamPos, Video_ChromaSubsampling).find(__T("4:2:2"))!=string::npos)
    {
        Node* Node_ColorSampling=Node_VisualCoding->Add_Child("mpeg7:ColorSampling");
        if (Extended)
            Node_ColorSampling->Add_Child("mpeg7:Name", std::string("YUV 4:2:2 Interlaced"));
        else
            Node_ColorSampling->XmlComment="YUV 4:2:2 Interlaced";
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
  
    //Language
    Ztring Language=MI.Get(Stream_Audio, StreamPos, Audio_Language);
    if (!Language.empty())
    {
        if (!Extended && StreamPos)
            Node_VisualCoding->Add_Child("mpeg7:language", Language);
        else
            Node_VisualCoding->Add_Child("")->XmlCommentOut="Language: "+Language.To_UTF8();
    }
 
    //Encryption
    Ztring Encryption=MI.Get(Stream_Video, StreamPos, Video_Encryption);
    if (!Encryption.empty())
    {
        if (Extended || StreamPos)
            Node_VisualCoding->Add_Child("mpeg7:Encryption", Encryption);
        else
            Node_VisualCoding->Add_Child("")->XmlCommentOut="Encryption: "+Encryption.To_UTF8();
    }

    if (!Extended && StreamPos)
        Node_VisualCoding->XmlCommentOut = "More than 1 track";
}

//---------------------------------------------------------------------------
void Mpeg7_Transform_Audio(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, size_t Extended, size_t Menu_Pos=(size_t)-1)
{
    Node* Node_AudioCoding=Parent->Add_Child("mpeg7:AudioCoding");

    //ID
    if (Extended)
        Mpeg7_Create_Id(Node_AudioCoding, "audio", StreamPos, Menu_Pos);

    //StreamID
    Mpeg7_Create_StreamID(Node_AudioCoding, Extended, MI, Stream_Audio, StreamPos);

    //Format
    Mpeg7_CS(Node_AudioCoding, "mpeg7:Format", "AudioCodingFormatCS", Mpeg7_AudioCodingFormatCS_termID, Mpeg7_AudioCodingFormatCS_Name, MI, StreamPos);

    //AudioChannels
    auto termID=Mpeg7_AudioPresentationCS_termID(MI, StreamPos);
    Ztring termID_Name=Mpeg7_AudioPresentationCS_Name(termID, MI, StreamPos);
    Ztring Channels=Mpeg7_StripExtraValues(MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_));
    if (!Channels.empty() && Channels.To_int32s())
    {
        size_t frontP=termID_Name.find(__T(" front"));
        size_t front=frontP==string::npos?0:termID_Name[frontP-1]-'0';
        size_t sideP=termID_Name.find(__T(" side"));
        size_t side=sideP==string::npos?0:termID_Name[sideP-1]-'0';
        size_t surroundP=termID_Name.find(__T(" surround"));
        size_t surround=surroundP==string::npos?0:termID_Name[surroundP-1]-'0';
        size_t rearP=termID_Name.find(__T(" back"));
        size_t rear=rearP==string::npos?0:termID_Name[rearP-1]-'0';
        size_t lfeP=termID_Name.find(__T("."));
        size_t lfe=lfeP==string::npos?0:termID_Name[lfeP+1]-'0';
        side+=surround; //No surround
        Node* AudioChannels=Node_AudioCoding->Add_Child("mpeg7:AudioChannels", Channels);
        if (front)
            AudioChannels->Add_Attribute("front", to_string(front));
        if (side)
            AudioChannels->Add_Attribute("side", to_string(side));
        if (rear)
            AudioChannels->Add_Attribute("rear", to_string(rear));
        if (lfe)
            AudioChannels->Add_Attribute("lfe", to_string(lfe));
    }

    //Sample
    Node* Node_Sample=Node_AudioCoding->Add_Child("mpeg7:Sample");

    Ztring Rate=Mpeg7_StripExtraValues(MI.Get(Stream_Audio, StreamPos, Audio_SamplingRate));
    if (!Rate.empty())
        Node_Sample->Add_Attribute("rate", Rate);

    Ztring bitsPer=Mpeg7_StripExtraValues(MI.Get(Stream_Audio, StreamPos, Audio_BitDepth));
    if (!bitsPer.empty())
        Node_Sample->Add_Attribute("bitsPer", bitsPer);

    //Emphasis
    if (MI.Get(Stream_Audio, StreamPos, Audio_Format)==__T("MPEG Audio"))
        Node_AudioCoding->Add_Child("mpeg7:Emphasis", Mpeg7_AudioEmphasis(MI, StreamPos));

    //Presentation
    Mpeg7_CS(Node_AudioCoding, "mpeg7:Presentation", "AudioPresentationCS", Mpeg7_AudioPresentationCS_termID, Mpeg7_AudioPresentationCS_Name, MI, StreamPos);

    //ChannelPositions
    Ztring ChannelLayout=MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout);
    if (!ChannelLayout.empty() && !termID)
    {
        if (Extended || StreamPos)
            Node_AudioCoding->Add_Child("mpeg7:ChannelLayout", ChannelLayout);
        else
            Node_AudioCoding->Add_Child("")->XmlCommentOut="ChannelLayout: "+ChannelLayout.To_UTF8();
    }
   
    //Language
    Ztring Language=MI.Get(Stream_Audio, StreamPos, Audio_Language);
    if (!Language.empty())
    {
        if (!Extended && StreamPos)
            Node_AudioCoding->Add_Child("mpeg7:language", Language);
        else
            Node_AudioCoding->Add_Child("")->XmlCommentOut="Language: "+Language.To_UTF8();
    }
   
    //Encryption
    Ztring Encryption=MI.Get(Stream_Audio, StreamPos, Audio_Encryption);
    if (!Encryption.empty())
    {
        if (Extended || StreamPos)
            Node_AudioCoding->Add_Child("mpeg7:Encryption", Encryption);
        else
            Node_AudioCoding->Add_Child("")->XmlCommentOut="Encryption: "+Encryption.To_UTF8();
    }

    if (StreamPos && !Extended)
        Node_AudioCoding->XmlCommentOut = "More than 1 track";
}

//---------------------------------------------------------------------------
void Mpeg7_Transform_Text(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, size_t& Extended, size_t Menu_Pos=(size_t)-1)
{
    Node* Node_TextualCoding=Parent->Add_Child("mpeg7:TextualCoding");

    //ID
    if (Extended)
        Mpeg7_Create_Id(Node_TextualCoding, "textual", StreamPos, Menu_Pos);

    //StreamID
    Mpeg7_Create_StreamID(Node_TextualCoding, Extended, MI, Stream_Text, StreamPos);

    //Format
    Mpeg7_CS(Node_TextualCoding, "mpeg7:Format", "TextualCodingFormatCS", Mpeg7_TextualCodingFormatCS_termID, Mpeg7_TextualCodingFormatCS_Name, MI, StreamPos);
   
    //Frame
    if (!MI.Get(Stream_Text, 0, Text_DisplayAspectRatio).empty()
     || !MI.Get(Stream_Text, 0, Text_Height).empty()
     || !MI.Get(Stream_Text, 0, Text_Width).empty()
     || !MI.Get(Stream_Text, 0, Text_FrameRate).empty()
     || !MI.Get(Stream_Text, 0, Text_FrameRate_Mode).empty())
    {
        Node* Node_Frame=Node_TextualCoding->Add_Child("mpeg7:Frame");
        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Text, 0, Text_DisplayAspectRatio, "aspectRatio");

        Ztring Height=Mpeg7_StripExtraValues(MI.Get(Stream_Text, 0, Text_Height));
        if (!Height.empty())
            Node_Frame->Add_Attribute("height", Height);

        Ztring Width=Mpeg7_StripExtraValues(MI.Get(Stream_Text, 0, Text_Width));
        if (!Width.empty())
            Node_Frame->Add_Attribute("width", Width);

        Node_Frame->Add_Attribute_IfNotEmpty(MI, Stream_Text, 0, Text_FrameRate, "rate");

        if (MI.Get(Stream_Text, 0, Text_FrameRate_Mode)==__T("VFR"))
            Node_Frame->Add_Attribute("variableRate", "true");
    }

    //Language
    Ztring Language=MI.Get(Stream_Text, StreamPos, Text_Language);
    if (!Language.empty())
    {
        bool Forced=MI.Get(Stream_Text, StreamPos, Text_Forced).empty();
        if (!Extended)
        {
            Node* Node_CaptionLanguage=Node_TextualCoding->Add_Child("mpeg7:language", Language);
            if (!MI.Get(Stream_Text, StreamPos, Text_Forced).empty())
                Node_CaptionLanguage->Add_Attribute("closed", "false");
        }
        else
            Node_TextualCoding->Add_Child("")->XmlCommentOut="Language: "+Language.To_UTF8()+(Forced?", open":"");
    }

    //TotalNumOfSamples
    Ztring TotalNumOfSamples=MI.Get(Stream_Text, StreamPos, Text_Events_Total);
    if (!TotalNumOfSamples.empty())
        Node_TextualCoding->Add_Child("mpeg7:TotalNumOfSamples", TotalNumOfSamples);

    //Encryption
    Ztring Encryption=MI.Get(Stream_Text, StreamPos, Text_Encryption);
    if (!Encryption.empty())
        Node_TextualCoding->Add_Child("mpeg7:Encryption", Encryption);

    if (!Extended)
        Node_TextualCoding->XmlCommentOut = "No Textual track in strict MPEG-7";
}

//---------------------------------------------------------------------------
void Mpeg7_Transform(Node* Node_MultimediaContent, MediaInfo_Internal& MI, size_t& Extended, size_t Menu_Pos=(size_t)-1)
{
    //(Type)
    Node* Node_Type=Node_MultimediaContent->Add_Child(Ztring(Ztring(__T("mpeg7:"))+Ztring(Mpeg7_Type(MI))).To_UTF8());

    //MediaFormat header
    Node* Node_MediaInformation=Node_Type->Add_Child("mpeg7:MediaInformation");
    Node* Node_MediaProfile=Node_MediaInformation->Add_Child("mpeg7:MediaProfile");

    Node* Node_MediaFormat=Node_MediaProfile->Add_Child("mpeg7:MediaFormat");

    //Content
    Mpeg7_CS(Node_MediaFormat, "mpeg7:Content", "ContentCS", Mpeg7_ContentCS_termID, Mpeg7_ContentCS_Name, MI, 0, true, true);

    //FileFormat
    if (Menu_Pos==(size_t)-1)
        Mpeg7_CS(Node_MediaFormat, "mpeg7:FileFormat", "FileFormatCS", Mpeg7_FileFormatCS_termID, Mpeg7_FileFormatCS_Name, MI, 0);

    //FileSize
    Node* FileSize=Node_MediaFormat->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_FileSize, "mpeg7:FileSize");
    if (FileSize && !MI.Get(Stream_General, 0, __T("IsTruncated")).empty())
    {
        if (Extended)
            FileSize->Add_Attribute("truncated", "true");
        else
            FileSize->XmlComment="Malformed file: truncated";
    }

    //System
    Mpeg7_CS(Node_MediaFormat, "mpeg7:System", "SystemCS", Mpeg7_SystemCS_termID, Mpeg7_SystemCS_Name, MI, 0);

    //BitRate
    Ztring BitRate=Mpeg7_StripExtraValues(MI.Get(Stream_General, 0, General_OverallBitRate));
    if (!BitRate.empty())
    {
        Node* Node_BitRate=Node_MediaFormat->Add_Child("mpeg7:BitRate", BitRate);
        Ztring BitRate_Mode=MI.Get(Stream_General, 0, General_OverallBitRate_Mode);
        bool IsCBR=BitRate_Mode==__T("CBR");
        bool IsVBR=BitRate_Mode==__T("VBR");
        if (!IsCBR && !IsVBR)
        {
            IsCBR=true;
            for (size_t StreamKind=Stream_Video; StreamKind<=Stream_Audio; StreamKind++)
                for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    BitRate_Mode=MI.Get((stream_t)StreamKind, StreamPos, __T("BitRate_Mode"));
                    if (BitRate_Mode!=__T("CBR"))
                        IsCBR=false;
                    if (BitRate_Mode==__T("VBR"))
                        IsVBR=true;
                }
        }
        if (IsCBR || IsVBR)
            Node_BitRate->Add_Attribute("variable", IsVBR?"true":"false");
    }

    //Filter in case of collections
    set<int64u> Set_Audio;
    set<int64u> Set_Text;
    if (Menu_Pos!=(size_t)-1)
    {
        for (auto& Field : FieldsToOffset)
        {
            auto FielFullName=string("List (")+Field+')';
            ZtringList List;
            List.Separator_Set(0, __T(" / "));
            List.Write(MI.Get(Stream_Menu, Menu_Pos, Ztring().From_UTF8(FielFullName.c_str())));
            for (auto& Item : List)
            {
                if (!Item.empty())
                {
                    auto Value=Item.To_int64u();
                    auto& Set=Field[0]=='A'?Set_Audio:Set_Text;
                    Set.insert(Value);
                }
            }
        }
    }

    //xxxCoding
    size_t Video_Count=MI.Count_Get(Stream_Video);
    size_t Audio_Count=MI.Count_Get(Stream_Audio);
    size_t Text_Count=MI.Count_Get(Stream_Text);
    for (size_t Pos=0; Pos<Video_Count; Pos++)
    {
        if (Menu_Pos!=(size_t)-1 && Pos!=Menu_Pos)
            continue;
        Mpeg7_Transform_Visual(Node_MediaFormat, MI, Pos, Extended, Menu_Pos);
    }
    for (size_t Pos=0; Pos<Audio_Count; Pos++)
    {
        if (Menu_Pos!=(size_t)-1 && Set_Audio.find(Pos)==Set_Audio.end())
            continue;
        Mpeg7_Transform_Audio(Node_MediaFormat, MI, Pos, Extended, Menu_Pos);
    }
    for (size_t Pos=0; Pos<Text_Count; Pos++)
    {
        if (Menu_Pos!=(size_t)-1 && Set_Text.find(Pos)==Set_Text.end())
            continue;
        Mpeg7_Transform_Text(Node_MediaFormat, MI, Pos, Extended, Menu_Pos);
    }

    //MediaTranscodingHints, intraFrameDistance and anchorFrameDistance
    if (!MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).empty()
     || !MI.Get(Stream_Video, 0, Video_Gop_OpenClosed).empty())
    {
        Ztring M=MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).SubString(__T("M="), __T(","));
        Ztring N=MI.Get(Stream_Video, 0, Video_Format_Settings_GOP).SubString(__T("N="), __T(""));
        Ztring OpenClosed=MI.Get(Stream_Video, 0, Video_Gop_OpenClosed);

        Node* Node_CodingHints=Node_MediaProfile->Add_Child("mpeg7:MediaTranscodingHints")->Add_Child("mpeg7:CodingHints");
        if (!N.empty())
            Node_CodingHints->Add_Attribute("intraFrameDistance", N);
        if (!M.empty())
            Node_CodingHints->Add_Attribute("anchorFrameDistance", M);
        if (OpenClosed==__T("Open") || OpenClosed==__T("Closed"))
        {
            if (OpenClosed==__T("Open"))
            {
                bool Gop_OpenClosed_FirstFrame=MI.Get(Stream_Video, 0, Video_Gop_OpenClosed_FirstFrame)==__T("Closed");
                if (Extended)
                {
                    Node_CodingHints->Add_Attribute("openGOP", "true");
                    if (Gop_OpenClosed_FirstFrame)
                        Node_CodingHints->Add_Attribute("openFirstGOP", "false");
                }
                else
                {
                    Node_CodingHints->XmlComment="openGOP: true";
                    if (Gop_OpenClosed_FirstFrame)
                        Node_CodingHints->XmlComment+=", openFirstGOP: false";
                }
            }
            else //Closed
            {
                if (Extended)
                    Node_CodingHints->Add_Attribute("openGOP", "false");
                else
                    Node_CodingHints->XmlComment="openGOP: false";
            }
        }
    }

    Node* Node_CreationInformation=nullptr;

    if (!MI.Get(Stream_General, 0, General_Movie).empty()
     || !MI.Get(Stream_General, 0, General_Part).empty()
     || !MI.Get(Stream_General, 0, General_Part_Position).empty()
     || !MI.Get(Stream_General, 0, General_Track).empty()
     || !MI.Get(Stream_General, 0, General_Track_Position).empty()
     || !MI.Get(Stream_General, 0, General_Album).empty()
     || !MI.Get(Stream_General, 0, General_Recorded_Date).empty()
     || !MI.Get(Stream_General, 0, General_Encoded_Date).empty()
     || !MI.Get(Stream_General, 0, General_Encoded_Application).empty()
     || !MI.Get(Stream_General, 0, General_Encoded_Library).empty()
     || !MI.Get(Stream_General, 0, General_Producer).empty()
     || !MI.Get(Stream_General, 0, General_Performer).empty())
    {
        if (!Node_CreationInformation)
            Node_CreationInformation=Node_Type->Add_Child("mpeg7:CreationInformation");
        Node* Node_Creation=Node_CreationInformation->Add_Child("mpeg7:Creation");

        if (!MI.Get(Stream_General, 0, General_Movie).empty()
         || !MI.Get(Stream_General, 0, General_Track).empty()
         || !MI.Get(Stream_General, 0, General_Track_Position).empty()
         || !MI.Get(Stream_General, 0, General_Part).empty()
         || !MI.Get(Stream_General, 0, General_Part_Position).empty()
         || !MI.Get(Stream_General, 0, General_Album).empty()
         || Menu_Pos!=(size_t)-1)
        {
            if (Menu_Pos!=(size_t)-1)
            {
                auto Title=std::to_string(Menu_Pos+1);
                if (Title.size()==1)
                    Title.insert(0, 1, '0');
                auto Source=MI.Get(Stream_Menu, Menu_Pos, __T("Source"));
                auto Part=Source.SubString(__T("VTS_"), __T("_0.IFO"));
                if (Part.size()==2 && Part[0]==__T('0'))
                    Part.erase(0, 1);
                Node_Creation->Add_Child("mpeg7:Title", "Title "+Title, "type", std::string("main"));
                if (!Part.empty())
                {
                    size_t Menu_Count=MI.Count_Get(Stream_Menu);
                    auto Source_Last=MI.Get(Stream_Menu, Menu_Count-1, __T("Source"));
                    auto Part_Last=Source_Last.SubString(__T("VTS_"), __T("_0.IFO"));
                    if (Part_Last.size()==2 && Part_Last[0]==__T('0'))
                        Part_Last.erase(0, 1);
                    if (!Part_Last.empty())
                    {
                        Part+=__T('/');
                        Part+=Part_Last;
                    }
                    Node_Creation->Add_Child("mpeg7:Title", Part.To_UTF8(), "type", std::string("urn:x-mpeg7-mediainfo:cs:TitleTypeCS:2009:PART"));
                    size_t Track=0;
                    size_t Track_Total=0;
                    for (size_t Pos=0; Pos<Menu_Count; Pos++)
                        if (MI.Get(Stream_Menu, Pos, __T("Source"))==Source)
                        {
                            if (Pos<=Menu_Pos)
                                Track++;
                            Track_Total++;
                        }
                    Node_Creation->Add_Child("mpeg7:Title", std::to_string(Track)+'/'+std::to_string(Track_Total), "type", std::string("urn:x-mpeg7-mediainfo:cs:TitleTypeCS:2009:TRACK"));
                }
            }
            else
                Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Movie, "mpeg7:Title", "type", std::string("main"));
            Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Track, "mpeg7:Title", "type", std::string("songTitle"));
            if (!MI.Get(Stream_General, 0, General_Part_Position).empty())
            {
                Ztring Total=MI.Get(Stream_General, 0, General_Part_Position_Total);
                Ztring Value=MI.Get(Stream_General, 0, General_Part_Position)+(Total.empty()?Ztring():(__T("/")+Total));

                Node_Creation->Add_Child("mpeg7:Title", Value, "type", std::string("urn:x-mpeg7-mediainfo:cs:TitleTypeCS:2009:PART"));
            }
            if (!MI.Get(Stream_General, 0, General_Track_Position).empty())
            {
                 Ztring Total=MI.Get(Stream_General, 0, General_Track_Position_Total);
                 Ztring Value=MI.Get(Stream_General, 0, General_Track_Position)+(Total.empty()?Ztring():(__T("/")+Total));

                 Node_Creation->Add_Child("mpeg7:Title", Value, "type", std::string("urn:x-mpeg7-mediainfo:cs:TitleTypeCS:2009:TRACK"));
            }
            Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Album, "mpeg7:Title", "type", std::string("albumTitle"));
        }
        else
        {
            Ztring FileName=MI.Get(Stream_General, 0, General_FileName);
            Ztring Extension=MI.Get(Stream_General, 0, General_FileExtension);
            if (!Extension.empty())
                FileName+=__T('.')+Extension;
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
        if (!MI.Get(Stream_General, 0, General_Recorded_Date).empty())
        {
            Node* Node_Date=Node_Creation->Add_Child("mpeg7:CreationCoordinates")->Add_Child("mpeg7:Date");
            Ztring TimePoint=Mpeg7_TimeToISO(MI.Get(Stream_General, 0, General_Recorded_Date));
            bool TimePoint_Isvalid=Mpeg7_TimeToISO_Isvalid(TimePoint);
            if (TimePoint_Isvalid)
                Node_Date->Add_Child("")->XmlCommentOut="Recorded date";
            Node_Date->Add_Child("mpeg7:TimePoint", TimePoint);
            if (!TimePoint_Isvalid)
                Node_Date->XmlCommentOut="Recorded date, invalid input";
        }
        if (!MI.Get(Stream_General, 0, General_Encoded_Date).empty())
        {
            Node* Node_Date=Node_Creation->Add_Child("mpeg7:CreationCoordinates")->Add_Child("mpeg7:Date");
            Ztring TimePoint=Mpeg7_TimeToISO(MI.Get(Stream_General, 0, General_Encoded_Date));
            bool TimePoint_Isvalid=Mpeg7_TimeToISO_Isvalid(TimePoint);
            if (TimePoint_Isvalid)
                Node_Date->Add_Child("")->XmlCommentOut="Encoded date";
            Node_Date->Add_Child("mpeg7:TimePoint", TimePoint);
            if (!TimePoint_Isvalid)
                Node_Date->XmlCommentOut="Encoded date, invalid input";
        }
        if (!MI.Get(Stream_General, 0, General_Producer).empty() && Menu_Pos==(size_t)-1)
        {
            Node* Node_Tool=Node_Creation->Add_Child("mpeg7:CreationTool")->Add_Child("mpeg7:Tool");
            Node_Tool->Add_Child("")->XmlCommentOut="Producer";
            Node_Tool->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Producer));
        }
        if (!MI.Get(Stream_General, 0, General_Encoded_Application).empty() && Menu_Pos==(size_t)-1)
        {
            Node* Node_Tool=Node_Creation->Add_Child("mpeg7:CreationTool")->Add_Child("mpeg7:Tool");
            Node_Tool->Add_Child("")->XmlCommentOut="Writing application";
            Node_Tool->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Encoded_Application));
        }
        else if (!MI.Get(Stream_General, 0, General_Encoded_Library).empty() && Menu_Pos==(size_t)-1)
        {
            Node* Node_Tool=Node_Creation->Add_Child("mpeg7:CreationTool")->Add_Child("mpeg7:Tool");
            Node_Tool->Add_Child("")->XmlCommentOut="Writing library";
            Node_Tool->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Encoded_Library));
        }
    }

    //Language
    ZtringList VideoLanguages, AudioLanguages, TextLanguages, AudioTypes, TextTypes;
    bool VideoLanguages_HasContent=false, AudioLanguages_HasContent=false, TextLanguages_HasContent=false;
    for (size_t StreamPos=0; StreamPos< Video_Count; StreamPos++)
    {
        Ztring Language=MI.Get(Stream_Video, StreamPos, Video_Language);
        if (Language.empty())
            Language=__T("und");
        else
            VideoLanguages_HasContent=true;
        VideoLanguages.push_back(Language);
    }
    for (size_t StreamPos=0; StreamPos<Audio_Count; StreamPos++)
    {
        Ztring Language=MI.Get(Stream_Audio, StreamPos, Audio_Language);
        if (Language.empty())
            Language=__T("und");
        else
            AudioLanguages_HasContent=true;
        AudioLanguages.push_back(Language);
        Ztring ServiceKind=MI.Get(Stream_Audio, StreamPos, Audio_ServiceKind);
        auto Type=Mpeg7_ServiceKind2Type_Get(ServiceKind.To_UTF8().c_str());
        if (Type)
            ServiceKind.From_UTF8(Type);
        else
            ServiceKind.clear();
        if (!ServiceKind.empty())
            AudioLanguages_HasContent=true;
        AudioTypes.push_back(ServiceKind);
    }
    for (size_t StreamPos=0; StreamPos<Text_Count; StreamPos++)
    {
        Ztring Language=MI.Get(Stream_Text, StreamPos, Text_Language);
        if (Language.empty())
            Language=__T("und");
        else
            TextLanguages_HasContent=true;
        TextLanguages.push_back(Language);
        Ztring ServiceKind=MI.Get(Stream_Text, StreamPos, Text_ServiceKind);
        auto Type=Mpeg7_ServiceKind2Type_Get(ServiceKind.To_UTF8().c_str());
        if (Type)
            ServiceKind.From_UTF8(Type);
        else
            ServiceKind.clear();
        if (!ServiceKind.empty())
            TextLanguages_HasContent=true;
        TextTypes.push_back(ServiceKind);
    }

    if (VideoLanguages_HasContent
     || AudioLanguages_HasContent
     || TextLanguages_HasContent
    )
    {
        if (!Node_CreationInformation)
            Node_CreationInformation=Node_Type->Add_Child("mpeg7:CreationInformation");
        Node* Node_Classification=Node_CreationInformation->Add_Child("mpeg7:Classification");

        if (VideoLanguages_HasContent)
        {
            for (size_t StreamPos=0; StreamPos<VideoLanguages.size(); StreamPos++)
            {
                if (Menu_Pos!=(size_t)-1 && StreamPos!=Menu_Pos)
                    continue;
                const auto& Language=VideoLanguages[StreamPos];
                if (Extended)
                {
                    auto Node_Language=Node_Classification->Add_Child("mpeg7:Language", Language);
                    Mpeg7_Create_Ref(Node_Language, "visual", StreamPos, Menu_Pos);
                }
                else
                    Node_Classification->Add_Child("")->XmlCommentOut="visual "+to_string(1+StreamPos)+" language: "+Language.To_UTF8();
            }
        }
        if (AudioLanguages_HasContent)
        {
            for (size_t StreamPos=0; StreamPos< AudioLanguages.size(); StreamPos++)
            {
                if (Menu_Pos!=(size_t)-1 && Set_Audio.find(StreamPos)==Set_Audio.end())
                    continue;
                const auto& Language=AudioLanguages[StreamPos];
                if (!Extended && (VideoLanguages_HasContent || AudioLanguages.size()>1))
                    Node_Classification->Add_Child("")->XmlCommentOut="below is audio languages with same order as in MediaFormat";
                auto Node_Language=Node_Classification->Add_Child("mpeg7:Language", Language);
                if (Extended)
                {
                    Mpeg7_Create_Ref(Node_Language, "audio", StreamPos, Menu_Pos);
                    if (!AudioTypes[StreamPos].empty())
                        
                        Node_Language->Add_Attribute("type", AudioTypes[StreamPos]);
                }
            }
        }
        if (TextLanguages_HasContent)
        {
            for (size_t StreamPos=0; StreamPos<TextLanguages.size(); StreamPos++)
            {
                if (Menu_Pos!=(size_t)-1 && Set_Text.find(StreamPos)==Set_Text.end())
                    continue;
                const auto& Language=TextLanguages[StreamPos];
                Node* Node_CaptionLanguage=Node_Classification->Add_Child("mpeg7:CaptionLanguage", Language);
                if (!MI.Get(Stream_Text, StreamPos, Text_Forced).empty())
                    Node_CaptionLanguage->Add_Attribute("closed", "false");
                if (Extended)
                {
                    Mpeg7_Create_Ref(Node_CaptionLanguage, "textual", StreamPos, Menu_Pos);
                    if (!TextTypes[StreamPos].empty())
                        Node_CaptionLanguage->Add_Attribute("type", TextTypes[StreamPos]);
                }
            }
        }
    }

    auto MediaTimePoint_Value=Mpeg7_MediaTimePoint(MI, Menu_Pos);
    auto MediaDuration_Value=Mpeg7_MediaDuration(MI, Menu_Pos);
    if (!MediaTimePoint_Value.empty() || !MediaDuration_Value.empty())
    {
        Node* Node_MediaTime=Node_Type->Add_Child("mpeg7:MediaTime");
        if (Menu_Pos!=(size_t)-1)
        {
            Ztring Source=MI.Get(Stream_Menu, Menu_Pos, __T("Source"));
            if (!Source.empty())
            {
                #ifdef WINDOWS
                Source.FindAndReplace(__T("\\"), __T("/"), Ztring_Recursive);
                #endif
                if (Extended)
                    Node_MediaTime->Add_Child("mpeg7:MediaLocator")->Add_Child("mpeg7:MediaUri", Source);
                else
                    Node_MediaTime->Add_Child("")->XmlCommentOut="MediaLocator: "+Source.To_UTF8();
            }
        }
        //MediaTimePoint
        if (MediaTimePoint_Value.empty())
            MediaTimePoint_Value="T00:00:00"; //It is mandatory, so fake value
        if (!MediaTimePoint_Value.empty())
            Node_MediaTime->Add_Child("mpeg7:MediaTimePoint", MediaTimePoint_Value);
        //MediaDuration
        if (!MediaDuration_Value.empty())
            Node_MediaTime->Add_Child("mpeg7:MediaDuration", MediaDuration_Value);
    }
}

//---------------------------------------------------------------------------
Ztring Export_Mpeg7::Transform(MediaInfo_Internal &MI, size_t Version)
{
    Ztring Value;

    //Version upgrade if needed
    size_t Video_Count=MI.Count_Get(Stream_Video);
    size_t Audio_Count=MI.Count_Get(Stream_Audio);
    size_t Text_Count=MI.Count_Get(Stream_Text);
    size_t Extended=0;
    switch (Version)
    {
        case Export_Mpeg7::Version_Extended:
            Extended=1;
            break;
        case Export_Mpeg7::Version_BestEffort_Extended:
            if (MI.Get(Stream_Video, 0, Video_FrameRate_Mode)==__T("VFR")
             || !MI.Get(Stream_Video, 0, Video_Gop_OpenClosed).empty()
             || !MI.Get(Stream_Video, 0, Video_Language).empty()
             || (!Mpeg7_AudioPresentationCS_termID(MI, 0) && !MI.Get(Stream_Audio, 0, Audio_ChannelLayout).empty()))
                Extended=1;
            //fall through
        case Export_Mpeg7::Version_BestEffort_Strict:
            if (Video_Count>1
             || Audio_Count>1
             || Text_Count
             || !MI.Get(Stream_Video, 0, Video_Encryption).empty()
             || !MI.Get(Stream_Audio, 0, Audio_Encryption).empty()
             || !MI.Get(Stream_General, 0, __T("IsTruncated")).empty())
                Extended=1;
    }

    //ebuCoreMain
    Node* Node_Mpeg7 = new Node("mpeg7:Mpeg7");
    Node_Mpeg7->Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    if (Extended)
    {
        Node_Mpeg7->Add_Attribute("xmlns", "urn:mpeg:mpeg7-extended:schema:2023");
        Node_Mpeg7->Add_Attribute("xmlns:mpeg7", "urn:mpeg:mpeg7-extended:schema:2023");
        Node_Mpeg7->Add_Attribute("xsi:schemaLocation", "urn:mpeg:mpeg7-extended:schema:2023 https://mediaarea.net/xsd/mpeg7-v2-extended.xsd");
    }
    else
    {
        Node_Mpeg7->Add_Attribute("xmlns", "urn:mpeg:mpeg7:schema:2004");
        Node_Mpeg7->Add_Attribute("xmlns:mpeg7", "urn:mpeg:mpeg7:schema:2004");
        Node_Mpeg7->Add_Attribute("xsi:schemaLocation", "urn:mpeg:mpeg7:schema:2004 http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-7_schema_files/mpeg7-v2.xsd");
    }

    //Description - DescriptionMetadata
    Node* Node_DescriptionMetadata=Node_Mpeg7->Add_Child("mpeg7:DescriptionMetadata");

    Node_DescriptionMetadata->Add_Child_IfNotEmpty(MI, Stream_General, 0, "ISAN", "mpeg7:PublicIdentifier", "type", std::string("ISAN"));
    Node_DescriptionMetadata->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_ISRC, "mpeg7:PublicIdentifier", "type", std::string("ISRC"));
    Node_DescriptionMetadata->Add_Child_IfNotEmpty(MI, Stream_General, 0, "Producer_Reference", "mpeg7:PublicIdentifier", "type", std::string("OriginatorReference"));
    Ztring UMID=MI.Get(Stream_General, 0, __T("UMID"));
    if (!UMID.empty())
    {
        if (UMID.size()>1 && UMID[0]==__T('0') && UMID[1]==__T('x'))
            UMID.erase(0, 2);
        Node_DescriptionMetadata->Add_Child("mpeg7:PublicIdentifier", UMID, "type", std::string("UMID"));
    }

    Ztring FileName=MI.Get(Stream_General, 0, General_FileName);
    Ztring Extension=MI.Get(Stream_General, 0, General_FileExtension);
    if (!Extension.empty())
        FileName+=__T('.')+Extension;
    if (!FileName.empty())
       Node_DescriptionMetadata->Add_Child("mpeg7:PrivateIdentifier", FileName);

    //Current date/time is ISO format
    time_t Time=time(NULL);
    Ztring TimeS; TimeS.Date_From_Seconds_1970((int32u)Time);
    if (!TimeS.empty())
    {
        TimeS.FindAndReplace(__T("UTC "), __T(""));
        TimeS.FindAndReplace(__T(" "), __T("T"));
        TimeS+=__T("+00:00");
        Node_DescriptionMetadata->Add_Child("mpeg7:CreationTime", TimeS);
    }

    Node* Node_Instrument=Node_DescriptionMetadata->Add_Child("mpeg7:Instrument");
    Node_Instrument->Add_Child("mpeg7:Tool")->Add_Child("mpeg7:Name", MediaInfoLib::Config.Info_Version_Get());

    //Description - CreationDescription
    Node* Node_Description=Node_Mpeg7->Add_Child("mpeg7:Description", "", "xsi:type", "ContentEntityType");

    //MultimediaContent
    Node* Node_MultimediaContent;
    Node* Node_Type;
    auto Collection_Display=Config.Collection_Display_Get();
    if (Collection_Display>=display_if::Always || (Collection_Display>display_if::Never && MI.Get(Stream_General, 0, General_Format).find(__T("DVD Video"))!=string::npos && (Collection_Display>=display_if::Supported || MI.Count_Get(Stream_Menu)>1)))
    {
        Node_MultimediaContent=Node_Description->Add_Child("mpeg7:MultimediaContent", std::string(), "xsi:type", "MultimediaCollectionType");
        auto Node_Collection=Node_MultimediaContent->Add_Child("mpeg7:Collection", std::string(), "xsi:type", "ContentCollectionType");
        auto Node_CreationInformation=Node_Collection->Add_Child("mpeg7:CreationInformation");
        auto Node_Creation=Node_CreationInformation->Add_Child("mpeg7:Creation");
        Node_Creation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Movie, "mpeg7:Title", "type", std::string("main"));
        if (!MI.Get(Stream_General, 0, General_Encoded_Application).empty())
        {
            Node* Node_Tool=Node_Creation->Add_Child("mpeg7:CreationTool")->Add_Child("mpeg7:Tool");
            Node_Tool->Add_Child("mpeg7:Name", MI.Get(Stream_General, 0, General_Encoded_Application));
        }
        auto Node_TextAnnotation=Node_Collection->Add_Child("mpeg7:TextAnnotation");
        auto Node_KeywordAnnotation=Node_TextAnnotation->Add_Child("mpeg7:KeywordAnnotation");
        Node_KeywordAnnotation->Add_Child("mpeg7:Keyword", MI.Get(Stream_General, 0, General_Format), "type", "main");

        //Main parsing
        size_t Menu_Count=MI.Count_Get(Stream_Menu);
        for (size_t Menu_Pos=0; Menu_Pos<Menu_Count; Menu_Pos++)
        {
            auto Node_Collection_Content=Node_Collection->Add_Child("mpeg7:Content", std::string(), "xsi:type", Ztring(Ztring(Mpeg7_Type(MI))+__T("Type")).To_UTF8());
            Node_Collection_Content->Add_Attribute("id", "content."+to_string(1+Menu_Pos));
            Mpeg7_Transform(Node_Collection_Content, MI, Extended, Menu_Pos);
        }
    }
    else
    {
        Node_MultimediaContent=Node_Description->Add_Child("mpeg7:MultimediaContent", std::string(), "xsi:type", Ztring(Ztring(Mpeg7_Type(MI))+__T("Type")).To_UTF8());

        //Main parsing
        Mpeg7_Transform(Node_MultimediaContent, MI, Extended);
    }

    //Transform
    Ztring ToReturn=Ztring().From_UTF8(To_XML(*Node_Mpeg7, 0, true, true).c_str());

    //Find and replace
    ZtringListList ToReplace=MediaInfoLib::Config.Inform_Replace_Get_All();
    for (size_t Pos=0; Pos<ToReplace.size(); Pos++)
        ToReturn.FindAndReplace(ToReplace[Pos][0], ToReplace[Pos][1], 0, Ztring_Recursive);

    //Carriage return
    if (MediaInfoLib::Config.LineSeparator_Get()!=__T("\n"))
        ToReturn.FindAndReplace(__T("\n"), MediaInfoLib::Config.LineSeparator_Get(), 0, Ztring_Recursive);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif

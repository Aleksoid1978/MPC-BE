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
#if defined(MEDIAINFO_FIMS_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_Fims.h"
#include "MediaInfo/File__Analyse_Automatic.h"
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
Ztring Fims_XML_Encode (const Ztring& Data)
{
    Ztring Result;
    wstring::size_type Pos;
    for (Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case __T('"'): Result+=__T("&quot;"); break;
            case __T('&'): Result+=__T("&amp;"); break;
            case __T('\''): Result+=__T("&apos;"); break;
            case __T('<'): Result+=__T("&lt;"); break;
            case __T('>'): Result+=__T("&lg;"); break;
            default: Result+=Data[Pos];
        }
    }
     return Result;
}

//---------------------------------------------------------------------------
int32u Fims_VideoCompressionCodeCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
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

Ztring Fims_VideoCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case 1 : return __T("MPEG-1 Video");
        case 2 :    switch ((termID%10000)/100)
                    {
                        case 1 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Simple Profile @ Main Level");
                                        default: return __T("MPEG-2 Video Simple Profile");
                                    }
                        case 2 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Main Profile @ Low Level");
                                        case 2 : return __T("MPEG-2 Video Main Profile @ Main Level");
                                        case 3 : return __T("MPEG-2 Video Main Profile @ High 1440 Level");
                                        case 4 : return __T("MPEG-2 Video Main Profile @ High Level");
                                        default: return __T("MPEG-2 Video Main Profile");
                                    }
                        case 3 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video SNR Scalable Profile @ Low Level");
                                        case 2 : return __T("MPEG-2 Video SNR Scalable Profile @ Main Level");
                                        default: return __T("MPEG-2 Video SNR Scalable Profile");
                                    }
                        case 4 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Spatial Scalable Profile @ Main Level");
                                        case 2 : return __T("MPEG-2 Video Spatial Scalable Profile @ High 1440 Level");
                                        case 3 : return __T("MPEG-2 Video Spatial Scalable Profile @ High Level");
                                        default: return __T("MPEG-2 Video Spatial Scalable Profile");
                                    }
                        case 5 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video High Profile @ Main Level");
                                        case 2 : return __T("MPEG-2 Video High Profile @ High 1440 Level");
                                        case 3 : return __T("MPEG-2 Video High Profile @ High Level");
                                        default: return __T("MPEG-2 Video High Profile");
                                    }
                        case 6 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Multiview Profile @ Main Level");
                                        default: return __T("MPEG-2 Video Multiview Profile");
                                    }
                        case 7 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video 4:2:2 Profile @ Main Level");
                                        default: return __T("MPEG-2 Video 4:2:2 Profile");
                                    }
                        default: return __T("MPEG-2 Video");
                    }
        case 3 :    switch ((termID%10000)/100)
                    {
                        case  1 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Visual Simple Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Visual Simple Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Visual Simple Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Simple Profile");
                                    }
                        case  2 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Scalable Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Simple Scalable Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Simple Scalable Profile");
                                    }
                        case  3 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 3");
                                        case 5 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 4");
                                        case 6 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 5");
                                        default: return __T("MPEG-4 Advanced Visual Simple Profile");
                                    }
                        case  4 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Core Profile");
                                    }
                        case  5 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Core-Scalable Profile");
                                    }
                        case  6 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual AdvancedCore Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual AdvancedCore Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual AdvancedCore Profile");
                                    }
                        case  7 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Main Profile @ Level 2");
                                        case 2 : return __T("MPEG-4 Visual Main Profile @ Level 3");
                                        case 3 : return __T("MPEG-4 Visual Main Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Main Profile");
                                    }
                        case  8 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual N-bit Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Main Profile");
                                    }
                        case  9 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Advanced Real Time Simple Profile");
                                    }
                        case 10 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Advanced Coding Efficiency Profile");
                                    }
                        case 11 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Simple Studio Profile");
                                    }
                        case 12 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core Studio Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core Studio Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Core Studio Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Core Studio Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Core Studio Profile");
                                    }
                        case 13 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 3");
                                        case 5 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 4");
                                        case 6 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 5");
                                        default: return __T("MPEG-4 Visual Fine Granularity Scalable Profile");
                                    }
                        case 14 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Simple Face Animation Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Simple Face Animation Profile @ Level 2");
                                        default: return __T("MPEG-4 Simple Face Animation Profile");
                                    }
                        case 15 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Simple FBA Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Simple FBA Profile @ Level 2");
                                        default: return __T("MPEG-4 Simple FBA Profile");
                                    }
                        case 16 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Basic Animated Texture Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Basic Animated Texture Profile @ Level 2");
                                        default: return __T("MPEG-4 Basic Animated Texture Profile");
                                    }
                        case 17 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Advanced Scalable Texture Profile @ Level 1");
                                        default: return __T("MPEG-4 Advanced Scalable Texture Profile");
                                    }
                        case 18 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Advanced Scalable Texture Profile");
                                    }
                        case 19 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Hybrid Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Hybrid Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Hybrid Profile");
                                    }
                        default: return __T("MPEG-4 Visual");
                    }
        case 4 :    return __T("JPEG");
        case 5 :    return __T("MJPEG");
        case 6 :    return __T("JPEG2000");
        case 7 :    return __T("H261");
        case 8 :    return __T("H263");
        default: return MI.Get(Stream_Video, StreamPos, Video_Format);
    }
}

//---------------------------------------------------------------------------
int32u Fims_AudioCompressionCodeCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
{
    const Ztring &Format=MI.Get(Stream_Audio, StreamPos, Audio_Format);
    const Ztring &Version=MI.Get(Stream_Audio, StreamPos, Audio_Format_Version);
    const Ztring &Profile=MI.Get(Stream_Audio, StreamPos, Audio_Format_Profile);

    if (Format==__T("AC-3"))
        return 40200;
    if (Format==__T("E-AC-3"))
        return 40300;
    if (Format==__T("Dolby E"))
        return 40600;
    if (Format==__T("DTS"))
        return 50000;
    if (Format==__T("MPEG Audio"))
    {
        if (Version.find(__T('1'))!=string::npos)
        {
            if (Profile.find(__T('1'))!=string::npos)
                return 70100;
            if (Profile.find(__T('2'))!=string::npos)
                return 70200;
            if (Profile.find(__T('3'))!=string::npos)
                return 70300;
            return 70000;
        }
        if (Version.find(__T('2'))!=string::npos)
        {
            if (Profile.find(__T('1'))!=string::npos)
                return 90100;
            if (Profile.find(__T('2'))!=string::npos)
                return 90200;
            if (Profile.find(__T('3'))!=string::npos)
                return 90300;
            return 90000;
        }
        return 0;
    }
    if (Format==__T("PCM"))
        return 110000;

    return 0;
}

Ztring Fims_AudioCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case 4 :    switch ((termID%10000)/100)
                    {
                        case 2 : return __T("AC3");
                        case 3 : return __T("E-AC3");
                        case 6 : return __T("Dolby E");
                        default: return __T("Dolby");
                    }
        case 5 : return __T("DTS");
        case 7 :    switch ((termID%10000)/100)
                    {
                        case 1 : return __T("MPEG-1 Audio Layer I");
                        case 2 : return __T("MPEG-1 Audio Layer II");
                        case 3 : return __T("MPEG-1 Audio Layer III");
                        default: return __T("MPEG-1 Audio");
                    }
        case 9 :    switch ((termID%10000)/100)
                    {
                        case 1 : return __T("MPEG-2 Audio Layer I");
                        case 2 : return __T("MPEG-2 Audio Layer II");
                        case 3 : return __T("MPEG-2 Audio Layer III");
                        default: return __T("MPEG-2 Audio");
                    }
        default: return MI.Get(Stream_Audio, StreamPos, Video_Format);
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Export_Fims::Export_Fims ()
{
}

//---------------------------------------------------------------------------
Export_Fims::~Export_Fims ()
{
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
Ztring Fims_Transform_Video(Ztring &ToReturn, MediaInfo_Internal &MI, size_t StreamPos, Export_Fims::version Version)
{
    size_t As11_UkDpp_Pos=(size_t)-1;
    for (size_t StreamPos_Temp=0; StreamPos_Temp<MI.Count_Get(Stream_Other); StreamPos_Temp++)
    {
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 UKDPP"))
            As11_UkDpp_Pos=StreamPos_Temp;
    }

    ToReturn+=__T("\t\t\t\t<bms:videoFormat");
    //if (!MI.Get(Stream_Video, StreamPos, Video_ID).empty())
    //    ToReturn+=__T(" videoFormatId=\"")+MI.Get(Stream_Video, StreamPos, Video_ID)+__T("\"");
    ToReturn+=__T(">");
    if (!MI.Get(Stream_Video, StreamPos, Video_Format_Version).empty())
        ToReturn+=__T(" <!-- videoFormatVersionId=\"")+MI.Get(Stream_Video, StreamPos, Video_Format_Version)+__T("\" -->");
    ToReturn+=__T("\n");

    ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");

    //flag_3D
    if (!MI.Get(Stream_Video, StreamPos, Video_MultiView_Count).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:flag_3D>true</bms:flag_3D>\n");

    //technicalAttributeString - ActiveFormatDescription
    if (!MI.Get(Stream_Video, StreamPos, Video_ActiveFormatDescription).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"ActiveFormatDescription\">")+MI.Get(Stream_Video, StreamPos, Video_ActiveFormatDescription)+__T("</bms:technicalAttribute>\n");

    //technicalAttributeString - Standard
    if (!MI.Get(Stream_Video, StreamPos, Video_Standard).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"Standard\">")+MI.Get(Stream_Video, StreamPos, Video_Standard)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - ColorSpace
    if (!MI.Get(Stream_Video, StreamPos, Video_ColorSpace).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"ColorSpace\">")+MI.Get(Stream_Video, StreamPos, Video_ColorSpace)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - ChromaSubsampling
    if (!MI.Get(Stream_Video, StreamPos, Video_ChromaSubsampling).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"ChromaSubsampling\">")+MI.Get(Stream_Video, StreamPos, Video_ChromaSubsampling)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - colour_primaries
    if (!MI.Get(Stream_Video, StreamPos, __T("colour_primaries")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"colour_primaries\">")+MI.Get(Stream_Video, StreamPos, __T("colour_primaries"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - transfer_characteristics
    if (!MI.Get(Stream_Video, StreamPos, __T("transfer_characteristics")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"transfer_characteristics\">")+MI.Get(Stream_Video, StreamPos, __T("transfer_characteristics"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - matrix_coefficients
    if (!MI.Get(Stream_Video, StreamPos, __T("matrix_coefficients")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"matrix_coefficients\">")+MI.Get(Stream_Video, StreamPos, __T("matrix_coefficients"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - colour_range
    if (!MI.Get(Stream_Video, StreamPos, __T("colour_range")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"colour_range\">")+MI.Get(Stream_Video, StreamPos, __T("colour_range"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - StreamSize
    if (!MI.Get(Stream_Video, StreamPos, Video_StreamSize).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"integer\" typeLabel=\"StreamSize\"")+(Version>=Export_Fims::Version_1_2?Ztring(__T(" unit=\"byte\"")):Ztring())+__T(">")+MI.Get(Stream_Video, StreamPos, Video_StreamSize)+__T("</bms:technicalAttribute>")+(Version<Export_Fims::Version_1_2?Ztring(__T(" <!-- unit=\"byte\" -->")):Ztring())+__T("\n");
    }

    //technicalAttributeString - BitDepth
    if (!MI.Get(Stream_Video, StreamPos, Video_BitDepth).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"integer\" typeLabel=\"BitDepth\"")+(Version>=Export_Fims::Version_1_2?Ztring(__T(" unit=\"bit\"")):Ztring())+__T(">")+MI.Get(Stream_Video, StreamPos, Video_BitDepth)+__T("</bms:technicalAttribute>")+(Version<Export_Fims::Version_1_2?Ztring(__T(" <!-- unit=\"bit\" -->")):Ztring())+__T("\n");
    }

    //technicalAttributeString
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaManufacturer")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"FPAManufacturer\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaManufacturer"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaVersion")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"FPAVersion\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaVersion"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeBoolean - Format_Settings_CABAC
    if (MI.Get(Stream_Video, StreamPos, Video_Format)==__T("AVC") && !MI.Get(Stream_Video, StreamPos, Video_Format_Settings_CABAC).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"boolean\" typeLabel=\"CABAC\">")+Ztring(MI.Get(Stream_Video, StreamPos, Video_Format_Settings_CABAC)==__T("Yes")?__T("true"):__T("false"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeBoolean - Format_Settings_MBAFF
    if (MI.Get(Stream_Video, StreamPos, Video_Format)==__T("AVC") && !MI.Get(Stream_Video, StreamPos, Video_ScanType).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"boolean\" typeLabel=\"MBAFF\">")+Ztring(MI.Get(Stream_Video, StreamPos, Video_ScanType)==__T("MBAFF")?__T("true"):__T("false"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeBoolean - FpaPass
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaPass")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"boolean\" typeLabel=\"MBAFF\">")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("FpaPass"))==__T("Yes")?__T("true"):__T("false"))+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - VideoComments
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("VideoComments")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"VideoComments\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("VideoComments"))+__T("</bms:technicalAttribute>\n");
    }

    //width
    if (!MI.Get(Stream_Video, StreamPos, Video_Width).empty())
    {
        Ztring Width;
        if (!MI.Get(Stream_Video, StreamPos, Video_Width_Original).empty())
            Width=MI.Get(Stream_Video, StreamPos, Video_Width_Original);
        else
            Width=MI.Get(Stream_Video, StreamPos, Video_Width);
        ToReturn+=__T("\t\t\t\t\t<bms:displayWidth unit=\"pixel\">")+MI.Get(Stream_Video, StreamPos, Video_Width)+__T("</bms:displayWidth>\n");
    }

    //height
    if (!MI.Get(Stream_Video, StreamPos, Video_Height).empty())
    {
        Ztring Height;
        if (!MI.Get(Stream_Video, StreamPos, Video_Height_Original).empty())
            Height=MI.Get(Stream_Video, StreamPos, Video_Height_Original);
        else
            Height=MI.Get(Stream_Video, StreamPos, Video_Height);
        ToReturn+=__T("\t\t\t\t\t<bms:displayHeight unit=\"pixel\">")+Height+__T("</bms:displayHeight>\n");
    }

    //frameRate
    if (!MI.Get(Stream_Video, StreamPos, Video_FrameRate).empty())
    {
        Ztring FrameRateString=MI.Get(Stream_Video, StreamPos, Video_FrameRate);
        Ztring factorNumerator, factorDenominator;
        if (FrameRateString==__T("23.976"))
        {
            factorNumerator=__T("24000");
            factorDenominator=__T("1001");
        }
        if (FrameRateString==__T("29.970"))
        {
            factorNumerator=__T("30000");
            factorDenominator=__T("1001");
        }
        if (FrameRateString==__T("59.940"))
        {
            factorNumerator=__T("60000");
            factorDenominator=__T("1001");
        }
        if (factorNumerator.empty())
        {
            factorNumerator=Ztring::ToZtring(FrameRateString.To_float64()*1000, 0);
            factorDenominator=__T("1000");
        }
        ToReturn+=__T("\t\t\t\t\t<bms:frameRate");
        ToReturn+=__T(" numerator=\"")+factorNumerator+__T("\"");
        ToReturn+=__T(" denominator=\"")+factorDenominator+__T("\"");
        ToReturn+=__T(">")+Ztring::ToZtring(FrameRateString.To_float64(), 0);
        ToReturn+=__T("</bms:frameRate>\n");
    }

    //aspectRatio
    if (!MI.Get(Stream_Video, StreamPos, Video_DisplayAspectRatio).empty())
    {
        Ztring AspectRatioString=MI.Get(Stream_Video, StreamPos, Video_DisplayAspectRatio_String);
        size_t AspectRatioString_Pos=AspectRatioString.find(__T(':'));
        Ztring factorNumerator, factorDenominator;
        if (AspectRatioString_Pos!=(size_t)-1)
        {
            factorNumerator=AspectRatioString.substr(0, AspectRatioString_Pos);
            factorDenominator=AspectRatioString.substr(AspectRatioString_Pos+1);
        }
        if (factorNumerator.empty())
        {
            factorNumerator=Ztring::ToZtring(AspectRatioString.To_float64()*1000, 0);
            factorDenominator=__T("1000");
        }
        ToReturn+=__T("\t\t\t\t\t<bms:aspectRatio");
        ToReturn+=__T(" numerator=\"")+factorNumerator+__T("\"");
        ToReturn+=__T(" denominator=\"")+factorDenominator+__T("\"");
        ToReturn+=__T(">")+Ztring::ToZtring(MI.Get(Stream_Video, StreamPos, Video_DisplayAspectRatio).To_float64(), 0);
        ToReturn+=__T("</bms:aspectRatio>\n");
    }

    //videoEncoding
    //if (!MI.Get(Stream_Video, StreamPos, Video_Format_Profile).empty())
    {
        int32u TermID=Fims_VideoCompressionCodeCS_termID(MI, StreamPos);
        Ztring typeLabel;
        Ztring TermID_String;
        if (TermID)
        {
            typeLabel=Fims_VideoCompressionCodeCS_Name(TermID, MI, StreamPos);
            TermID_String=Ztring::ToZtring(TermID/10000);
            if (TermID%10000)
            {
                TermID_String+=__T('.');
                TermID_String+=Ztring::ToZtring((TermID%10000)/100);
                if (TermID%100)
                {
                    TermID_String+=__T('.');
                    TermID_String+=Ztring::ToZtring(TermID%100);
                }
            }
        }
        else
            typeLabel=MI.Get(Stream_Video, StreamPos, Video_Format_Profile);
        ToReturn+=__T("\t\t\t\t\t<bms:videoEncoding typeLabel=\"")+typeLabel+__T("\"");
        ToReturn+=__T(">\n");
        if (!MI.Get(Stream_Video, StreamPos, Video_Format).empty())
            ToReturn+=__T("\t\t\t\t\t\t<bms:family>")+MI.Get(Stream_Video, StreamPos, Video_Format)+__T("</bms:family>\n");
        if (!MI.Get(Stream_Video, StreamPos, Video_CodecID).empty())
        {
            ToReturn+=__T("\t\t\t\t\t\t<!-- <bms:identifier>")+MI.Get(Stream_Video, StreamPos, Video_CodecID)+__T("</dc:identifier> -->\n");
        }
        if (!MI.Get(Stream_Video, StreamPos, Video_Format_Commercial_IfAny).empty())
            ToReturn+=__T("\t\t\t\t\t\t<!-- <ebucore:name>")+MI.Get(Stream_Video, StreamPos, Video_Format_Commercial_IfAny)+__T("</ebucore:name> -->\n");
        ToReturn+=__T("\t\t\t\t\t</bms:videoEncoding>\n");
    }

    //videoTrack
    if (!MI.Get(Stream_Video, StreamPos, Video_ID).empty() || !MI.Get(Stream_Video, StreamPos, Video_Title).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:videoTrack");
        if (!MI.Get(Stream_Video, StreamPos, Video_Title).empty())
            ToReturn+=__T(" trackName=\"")+MI.Get(Stream_Video, StreamPos, Video_Title)+__T("\"");
        ToReturn+=__T("/>");
        if (!MI.Get(Stream_Video, StreamPos, Video_ID).empty())
            ToReturn+=__T(" <!-- trackId=\"")+MI.Get(Stream_Video, StreamPos, Video_ID)+__T("\" -->");
        ToReturn+=__T("\n");
    }

    //bitRate
    if (!MI.Get(Stream_Video, StreamPos, Video_BitRate).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:bitRate>")+MI.Get(Stream_Video, StreamPos, Video_BitRate)+__T("</bms:bitRate>\n");

    //bitRateMax
    if (!MI.Get(Stream_Video, StreamPos, Video_BitRate_Maximum).empty())
        ToReturn+=__T("\t\t\t\t\t <!-- <bms:bitRateMax>")+MI.Get(Stream_Video, StreamPos, Video_BitRate_Maximum)+__T("</bms:bitRateMax> -->\n");

    //bitRateMode
    if (!MI.Get(Stream_Video, StreamPos, Video_BitRate_Mode).empty())
    {
        Ztring bitRateMode=MI.Get(Stream_Video, StreamPos, Video_BitRate_Mode);
        if (bitRateMode==__T("CBR"))
            bitRateMode=__T("constant");
        if (bitRateMode==__T("VBR"))
            bitRateMode=__T("variable");
        ToReturn+=__T("\t\t\t\t\t<bms:bitRateMode>")+bitRateMode+__T("</bms:bitRateMode>\n");
    }

    //lines
    if (!MI.Get(Stream_Video, StreamPos, Video_Height_Original).empty())
        ToReturn+=__T("\t\t\t\t<bms:lines>")+MI.Get(Stream_Video, StreamPos, Video_Height_Original)+__T("</bms:lines>\n");

    //scanningFormat
    if (!MI.Get(Stream_Video, StreamPos, Video_ScanType).empty())
    {
        Ztring ScanType=MI.Get(Stream_Video, StreamPos, Video_ScanType);
        if (ScanType==__T("MBAFF"))
            ScanType=__T("Interlaced");
        ScanType.MakeLowerCase();
        ToReturn+=__T("\t\t\t\t\t<bms:scanningFormat>")+ScanType+__T("</bms:scanningFormat>\n");
     }

    //scanningOrder
    if (!MI.Get(Stream_Video, StreamPos, Video_ScanOrder).empty())
    {
        Ztring ScanOrder=MI.Get(Stream_Video, StreamPos, Video_ScanOrder);
        if (ScanOrder==__T("TFF"))
            ScanOrder=__T("top");
        if (ScanOrder==__T("BFF"))
            ScanOrder=__T("bottom");
        if (ScanOrder.find(__T("Pulldown"))!=string::npos)
            ScanOrder=__T("pulldown");
        ToReturn+=__T("\t\t\t\t\t<bms:scanningOrder>")+ScanOrder+__T("</bms:scanningOrder>\n");
    }

    ToReturn+=__T("\t\t\t\t</bms:videoFormat>\n");

    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Fims_Transform_Audio(Ztring &ToReturn, MediaInfo_Internal &MI, size_t StreamPos, Export_Fims::version Version)
{

    size_t As11_Core_Pos=(size_t)-1;
    size_t As11_UkDpp_Pos=(size_t)-1;
    for (size_t StreamPos_Temp=0; StreamPos_Temp<MI.Count_Get(Stream_Other); StreamPos_Temp++)
    {
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 Core"))
            As11_Core_Pos=StreamPos_Temp;
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 UKDPP"))
            As11_UkDpp_Pos=StreamPos_Temp;
    }

    ToReturn+=__T("\t\t\t\t<bms:audioFormat");
    //if (!MI.Get(Stream_Audio, StreamPos, Audio_ID).empty())
    //    ToReturn+=__T(" audioFormatId=\"")+MI.Get(Stream_Audio, StreamPos, Audio_ID)+__T("\"");
    ToReturn+=__T(">");
    if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Version).empty())
        ToReturn+=__T(" <!-- audioFormatVersionId=\"")+MI.Get(Stream_Audio, StreamPos, Audio_Format_Version)+__T("\" -->");
    ToReturn+=__T("\n");

    ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");

    //format - technicalAttributeString - ChannelPositions
    if (!MI.Get(Stream_Audio, StreamPos, Audio_ChannelPositions).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"ChannelPositions\">")+MI.Get(Stream_Audio, StreamPos, Audio_ChannelPositions)+__T("</bms:technicalAttribute>\n");
    }

    //format - technicalAttributeString - ChannelLayout
    if (!MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"ChannelLayout\">")+MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - Format_Settings_Endianness
    if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Settings_Endianness).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"Endianness\">")+MI.Get(Stream_Audio, StreamPos, Audio_Format_Settings_Endianness)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - Format_Settings_Wrapping
    if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Settings_Wrapping).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"Wrapping\">")+MI.Get(Stream_Audio, StreamPos, Audio_Format_Settings_Wrapping)+__T("</bms:technicalAttribute>\n");
    }

    //technicalAttributeString - StreamSize
    if (!MI.Get(Stream_Audio, StreamPos, Audio_StreamSize).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"integer\" typeLabel=\"StreamSize\"")+(Version>=Export_Fims::Version_1_2?Ztring(__T(" unit=\"byte\"")):Ztring())+__T(">")+MI.Get(Stream_Audio, StreamPos, Audio_StreamSize)+__T("</bms:technicalAttribute>")+(Version<Export_Fims::Version_1_2?Ztring(__T(" <!-- unit=\"byte\" -->")):Ztring())+__T("\n");
    }

    //technicalAttributeString
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioComments")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:comment typeLabel=\"AudioComments\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioComments"))+__T("</bms:comment>\n");
    }

    //samplingRate
    if (!MI.Get(Stream_Audio, StreamPos, Audio_SamplingRate).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:samplingRate>")+MI.Get(Stream_Audio, StreamPos, Audio_SamplingRate)+__T("</bms:samplingRate>\n");

    //audioEncoding
    //if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Profile).empty())
    {
        int32u TermID=Fims_AudioCompressionCodeCS_termID(MI, StreamPos);
        Ztring typeLabel;
        Ztring TermID_String;
        if (TermID)
        {
            typeLabel=Fims_AudioCompressionCodeCS_Name(TermID, MI, StreamPos);
            TermID_String=Ztring::ToZtring(TermID/10000);
            if (TermID%10000)
            {
                TermID_String+=__T('.');
                TermID_String+=Ztring::ToZtring((TermID%10000)/100);
                if (TermID%100)
                {
                    TermID_String+=__T('.');
                    TermID_String+=Ztring::ToZtring(TermID%100);
                }
            }
        }
        else
            typeLabel=MI.Get(Stream_Audio, StreamPos, Audio_Format_Profile);
        ToReturn+=__T("\t\t\t\t\t<bms:audioEncoding typeLabel=\"")+typeLabel+__T("\">\n");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_Format).empty())
            ToReturn+=__T("\t\t\t\t\t\t<bms:family>")+MI.Get(Stream_Audio, StreamPos, Audio_Format)+__T("</bms:family>\n");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_CodecID).empty())
        {
            ToReturn+=__T("\t\t\t\t\t\t<!-- <dc:identifier>")+MI.Get(Stream_Audio, StreamPos, Audio_CodecID)+__T("</dc:identifier> -->\n");
        }
        if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Commercial_IfAny).empty())
            ToReturn+=__T("\t\t\t\t\t\t<!-- <ebucore:name>")+MI.Get(Stream_Audio, StreamPos, Audio_Format_Commercial_IfAny)+__T("</ebucore:name> -->\n");
        ToReturn+=__T("\t\t\t\t\t</bms:audioEncoding>\n");
    }

    //audioTrackConfiguration
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("AudioTrackLayout")).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:trackConfiguration typeLabel=\"")+MI.Get(Stream_Other, As11_Core_Pos, __T("AudioTrackLayout"))+__T("\"/>\n");
    }

    //audioTrack
    if (!MI.Get(Stream_Audio, StreamPos, Audio_ID).empty() || !MI.Get(Stream_Audio, StreamPos, Audio_Title).empty() || !MI.Get(Stream_Audio, StreamPos, Audio_Language).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<bms:audioTrack");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_Language).empty())
            ToReturn+=__T(" trackLanguage=\"")+MI.Get(Stream_Audio, StreamPos, Audio_Language)+__T("\"");
        ToReturn+=__T("/>");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_ID).empty())
        {
            Ztring ID=MI.Get(Stream_Audio, StreamPos, Audio_ID);
            ID.FindAndReplace(__T(" / "), __T("_"));
            ToReturn+=__T(" <!-- trackId=\"")+ID+__T("\" -->");
        }
        ToReturn+=__T("\n");
   }

    //channels
    if (!MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:channels>")+MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_)+__T("</bms:channels>\n");

    //bitRate
    if (!MI.Get(Stream_Audio, StreamPos, Audio_BitRate).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:bitRate>")+MI.Get(Stream_Audio, StreamPos, Audio_BitRate)+__T("</bms:bitRate>\n");

    //bitRateMax
    if (!MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Maximum).empty())
        ToReturn+=__T("\t\t\t\t\t<-- <bms:bitRateMax>")+MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Maximum)+__T("</bms:bitRateMax> -->\n");

    //bitRateMode
    if (!MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Mode).empty())
    {
        Ztring bitRateMode=MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Mode);
        if (bitRateMode==__T("CBR"))
            bitRateMode=__T("constant");
        if (bitRateMode==__T("VBR"))
            bitRateMode=__T("variable");
        ToReturn+=__T("\t\t\t\t\t<bms:bitRateMode>")+bitRateMode+__T("</bms:bitRateMode>\n");
    }

    //sampleSize
    if (!MI.Get(Stream_Audio, StreamPos, Audio_BitDepth).empty())
        ToReturn+=__T("\t\t\t\t\t<bms:sampleSize>")+MI.Get(Stream_Audio, StreamPos, Audio_BitDepth)+__T("</bms:sampleSize>\n");

    ToReturn+=__T("\t\t\t\t</bms:audioFormat>\n");

    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Fims_Transform_Text(Ztring &ToReturn, MediaInfo_Internal &MI, size_t StreamPos)
{
    ToReturn+=__T("\t\t\t\t<!-- (Not implemented)\n");

    ToReturn+=__T("\t\t\t\t<ebucore:dataFormat");
    //if (!MI.Get(Stream_Text, StreamPos, Text_ID).empty())
    //    ToReturn+=__T(" dataFormatId=\"")+MI.Get(Stream_Text, StreamPos, Text_ID)+__T("\"");
    if (!MI.Get(Stream_Text, StreamPos, Text_Format_Version).empty())
        ToReturn+=__T(" dataFormatVersionId=\"")+MI.Get(Stream_Text, StreamPos, Text_Format_Version)+__T("\"");
    if (!MI.Get(Stream_Text, StreamPos, Text_Format).empty())
        ToReturn+=__T(" dataFormatName=\"")+MI.Get(Stream_Text, StreamPos, Text_Format)+__T("\"");
    if (!MI.Get(Stream_Text, StreamPos, Text_ID).empty())
        ToReturn+=__T(" dataTrackId=\"")+MI.Get(Stream_Text, StreamPos, Text_ID)+__T("\"");
    ToReturn+=__T(">\n");

    //subtitlingTrack
    {
        ToReturn+=__T("\t\t\t\t\t<ebucore:captioningFormat");
        if (!MI.Get(Stream_Text, StreamPos, Text_Format).empty())
            ToReturn+=__T(" captioningFormatName=\"")+MI.Get(Stream_Text, StreamPos, Text_Format)+__T("\"");
        if (!MI.Get(Stream_Text, StreamPos, Text_ID).empty())
            ToReturn+=__T(" trackId=\"")+MI.Get(Stream_Text, StreamPos, Text_ID)+__T("\"");
        if (!MI.Get(Stream_Text, StreamPos, Text_Title).empty())
            ToReturn+=__T(" typeLabel=\"")+MI.Get(Stream_Text, StreamPos, Text_Title)+__T("\"");
        if (!MI.Get(Stream_Text, StreamPos, Text_Language).empty())
            ToReturn+=__T(" language=\"")+MI.Get(Stream_Text, StreamPos, Text_Language)+__T("\"");
        ToReturn+=__T("/>\n");
        if (!MI.Get(Stream_Text, StreamPos, Text_CodecID).empty() || !MI.Get(Stream_Text, StreamPos, Text_Format_Commercial_IfAny).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<ebucore:codec>\n");
            if (!MI.Get(Stream_Text, StreamPos, Text_CodecID).empty())
            {
                ToReturn+=__T("\t\t\t\t\t\t<ebucore:codecIdentifier>\n");
                ToReturn+=__T("\t\t\t\t\t\t\t<dc:identifier>")+MI.Get(Stream_Text, StreamPos, Text_CodecID)+__T("</dc:identifier>\n");
                ToReturn+=__T("\t\t\t\t\t\t</ebucore:codecIdentifier>\n");
            }
            if (!MI.Get(Stream_Text, StreamPos, Text_Format_Commercial_IfAny).empty())
                ToReturn+=__T("\t\t\t\t\t\t<ebucore:name>")+MI.Get(Stream_Text, StreamPos, Text_Format_Commercial_IfAny)+__T("</ebucore:name>\n");
            ToReturn+=__T("\t\t\t\t\t</ebucore:codec>\n");
        }
    }

    ToReturn+=__T("\t\t\t\t</ebucore:dataFormat>\n");

    ToReturn+=__T("\t\t\t\t-->\n");

    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Fims_Transform_TimeCode(Ztring &ToReturn, MediaInfo_Internal &MI, size_t StreamPos, bool Is1_2)
{
    if (Is1_2)
        ToReturn+=__T("\t\t\t\t<!-- (timecodeFormat not in XSD)\n");

    ToReturn+=__T("\t\t\t\t<ebucore:timecodeFormat");
    if (!MI.Get(Stream_Other, StreamPos, Other_Format).empty())
        ToReturn+=__T(" timecodeFormatName=\"")+MI.Get(Stream_Other, StreamPos, Other_Format)+__T("\"");
    ToReturn+=__T(">\n");

    //start
    {
        ToReturn+=__T("\t\t\t\t\t<ebucore:timecodeStart>\n");
        ToReturn+=__T("\t\t\t\t\t\t<ebucore:timecode>")+MI.Get(Stream_Other, StreamPos, Other_TimeCode_FirstFrame)+__T("</ebucore:timecode>\n");
        ToReturn+=__T("\t\t\t\t\t</ebucore:timecodeStart>\n");
    }

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty() || !MI.Get(Stream_Other, StreamPos, Other_Title).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<ebucore:timecodeTrack");
        if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty())
        {
            Ztring ID=MI.Get(Stream_Other, StreamPos, Other_ID);
            if (MI.Get(Stream_Other, StreamPos, Other_ID).find(__T("-Material"))!=string::npos)
            {
                ID.FindAndReplace(__T("-Material"), Ztring());
                ToReturn+=__T(" trackId=\"")+ID+__T("\"")+__T(" typeLabel=\"Material\"");
            }
            else if (MI.Get(Stream_Other, StreamPos, Other_ID).find(__T("-Source"))!=string::npos)
            {
                ID.FindAndReplace(__T("-Source"), Ztring());
                ToReturn+=__T(" trackId=\"")+ID+__T("\"")+__T(" typeLabel=\"Source\"");
            }
            else
                ToReturn+=__T(" trackId=\"")+ID+__T("\"");
        }
        if (!MI.Get(Stream_Other, StreamPos, Other_Title).empty())
            ToReturn+=__T(" trackName=\"")+MI.Get(Stream_Other, StreamPos, Other_Title)+__T("\"");
        ToReturn+=__T("/>\n");
    }

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty())
        ToReturn+=__T("\t\t\t\t\t<ebucore:technicalAttributeBoolean typeLabel=\"Stripped\">")+Ztring(MI.Get(Stream_Other, StreamPos, __T("TimeCode_Striped"))==__T("Yes")?__T("true"):__T("false"))+__T("</ebucore:technicalAttributeBoolean>\n");

    ToReturn+=__T("\t\t\t\t</ebucore:timecodeFormat>\n");

    if (Is1_2)
        ToReturn+=__T("\t\t\t\t-->\n");

    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Fims_Transform_Metadata(Ztring &ToReturn, MediaInfo_Internal &MI, size_t StreamPos, bool Is1_2)
{
    if (Is1_2)
        ToReturn+=__T("\t\t\t\t<!-- (metadataFormat not in XSD)\n");

    ToReturn+=__T("\t\t\t\t<ebucore:metadataFormat");
    if (!MI.Get(Stream_Other, StreamPos, Other_Format).empty())
        ToReturn+=__T(" metadataFormatName=\"")+MI.Get(Stream_Other, StreamPos, Other_Format)+__T("\"");
    ToReturn+=__T(">\n");

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty() || !MI.Get(Stream_Other, StreamPos, Other_Title).empty())
    {
        ToReturn+=__T("\t\t\t\t\t<ebucore:metadataTrack");
        if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty())
            ToReturn+=__T(" trackId=\"")+MI.Get(Stream_Other, StreamPos, Other_ID)+__T("\"");
        if (!MI.Get(Stream_Other, StreamPos, Other_Title).empty())
            ToReturn+=__T(" trackName=\"")+MI.Get(Stream_Other, StreamPos, Other_Title)+__T("\"");
        ToReturn+=__T("/>\n");
    }

    ToReturn+=__T("\t\t\t\t</ebucore:metadataFormat>\n");

    if (Is1_2)
        ToReturn+=__T("\t\t\t\t-->\n");

    return ToReturn;
}

//---------------------------------------------------------------------------
Ztring Export_Fims::Transform(MediaInfo_Internal &MI, version Version)
{
    //Current date/time is ISO format
    time_t Seconds=time(NULL);
    Ztring DateTime; DateTime.Date_From_Seconds_1970((int32u)Seconds);
    DateTime.FindAndReplace(__T("UTC "), __T(""));
    DateTime.FindAndReplace(__T(" "), __T("T"));
    Ztring Date=DateTime.substr(0, 10);
    Ztring Time=DateTime.substr(11, 8);

    size_t As11_Core_Pos=(size_t)-1;
    size_t As11_Segmentation_Pos=(size_t)-1;
    size_t As11_UkDpp_Pos=(size_t)-1;
    for (size_t StreamPos_Temp=0; StreamPos_Temp<MI.Count_Get(Stream_Other); StreamPos_Temp++)
    {
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 Core"))
            As11_Core_Pos=StreamPos_Temp;
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 Segmentation"))
            As11_Segmentation_Pos=StreamPos_Temp;
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 UKDPP"))
            As11_UkDpp_Pos=StreamPos_Temp;
    }

    Ztring ToReturn;

    //bmContent
    ToReturn+=__T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    ToReturn+=__T("<!-- Generated by ")+MediaInfoLib::Config.Info_Version_Get()+__T(" -->\n");
    ToReturn+=__T("<!-- Based on EBUCore -->\n");
    ToReturn+=__T("<!-- Preliminary version (not for production) -->\n");
    ToReturn+=__T("<!-- dateLastModified=\"")+Date+__T("\" timeLastModified=\"")+Time+__T("Z\" -->\n");
    if (Version==Version_1_1)
        ToReturn+=__T("<bms:bmContent xmlns:desc=\"http://description.fims.tv\" xmlns:tim=\"http://baseTime.fims.tv\"\n    xmlns:bms=\"http://base.fims.tv\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n    xsi:schemaLocation=\"http://base.fims.tv http://mediaarea.net/temp/baseMediaService-V1_1_0.xsd\">\n");
    if (Version==Version_1_2)
        ToReturn+=__T("<bms:bmContent xmlns:desc=\"http://description.fims.tv\" xmlns:tim=\"http://baseTime.fims.tv\"\n    xmlns:bms=\"http://base.fims.tv\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n    xsi:schemaLocation=\"http://base.fims.tv http://mediaarea.net/temp/baseMediaService-V1_2_0.xsd\">\n");
    if (Version==Version_1_3)
        ToReturn+=__T("<bms:bmContent xmlns:desc=\"http://description.fims.tv\" xmlns:tim=\"http://baseTime.fims.tv\"\n    xmlns:bms=\"http://base.fims.tv\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n    xsi:schemaLocation=\"http://base.fims.tv http://mediaarea.net/temp/baseMediaService-V1_3_0.xsd\">\n");
    ToReturn+=__T("\t<bms:resourceID/>\n");

    //bmContentFormats
    ToReturn+=__T("\t<bms:bmContentFormats>\n");
    ToReturn+=__T("\t\t<bms:bmContentFormat>\n");
    ToReturn+=__T("\t\t\t<bms:resourceID/>\n");

    //bmContentFormats - resourceCreationDate
    if (!MI.Get(Stream_General, 0, General_Encoded_Date).empty())
    {
        Ztring DateTime=MI.Get(Stream_General, 0, General_Encoded_Date);
        DateTime.FindAndReplace(__T("UTC "), __T(""));
        DateTime.FindAndReplace(__T(" "), __T("T"));
        Ztring Date=DateTime.substr(0, 10);
        Ztring Time=DateTime.substr(11, 8);

        ToReturn+=__T("\t\t\t<bms:resourceCreationDate>")+Date+__T('T')+Time+__T("Z</bms:resourceCreationDate>\n");
    }

    //bmContentFormats - dateModified
    if (!MI.Get(Stream_General, 0, General_Tagged_Date).empty())
    {
        Ztring DateTime=MI.Get(Stream_General, 0, General_Tagged_Date);
        DateTime.FindAndReplace(__T("UTC "), __T(""));
        DateTime.FindAndReplace(__T(" "), __T("T"));
        Ztring Date=DateTime.substr(0, 10);
        Ztring Time=DateTime.substr(11, 8);

        ToReturn+=__T("\t\t\t<bms:resourceModifiedDate>")+Date+__T('T')+Time+__T("Z</bms:resourceModifiedDate>\n");
    }

    //bmContentFormats - bmEssenceLocators
    if (!MI.Get(Stream_General, 0, General_CompleteName).empty())
    {
        ToReturn+=__T("\t\t\t<bms:bmEssenceLocators>\n");
        ToReturn+=__T("\t\t\t\t<bms:bmEssenceLocator xsi:type=\"bms:EssencePlaceholderLocatorType\">\n");
        ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");
        ToReturn+=__T("\t\t\t\t\t<bms:locatorInfo>")+Fims_XML_Encode(MI.Get(Stream_General, 0, General_CompleteName))+__T("</bms:locatorInfo>\n");
        ToReturn+=__T("\t\t\t\t</bms:bmEssenceLocator>\n");
        ToReturn+=__T("\t\t\t</bms:bmEssenceLocators>\n");
    }

    //bmContentFormats - per stream
    {
        ToReturn+=__T("\t\t\t<bms:formatCollection>\n");

        //videoFormat
        for (size_t Pos=0; Pos<MI.Count_Get(Stream_Video); Pos++)
        {
            if (Version<Version_1_2 && Pos==1)
            {
                ToReturn+=__T("\t\t\t\t<!-- Not valid in XSD\n");
                ToReturn+=__T("\t\t\t\t")+Ztring::ToZtring(MI.Count_Get(Stream_Video)-1)+__T(" additional video streams\n");
                ToReturn+=__T("\t\t\t\t-->\n");
                break;
            }
            Fims_Transform_Video(ToReturn, MI, Pos, Version);
        }

        //audioFormat
        for (size_t Pos=0; Pos<MI.Count_Get(Stream_Audio); Pos++)
        {
            if (Version<Version_1_2 && Pos==1)
            {
                ToReturn+=__T("\t\t\t\t<!-- Not valid in XSD\n");
                ToReturn+=__T("\t\t\t\t")+Ztring::ToZtring(MI.Count_Get(Stream_Audio)-1)+__T(" additional audio streams\n");
                ToReturn+=__T("\t\t\t\t-->\n");
                break;
            }
            Fims_Transform_Audio(ToReturn, MI, Pos, Version);
        }

        //format - SigningPresent
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SigningPresent")).empty())
        {
            ToReturn+=__T("\t\t\t\t<bms:dataFormat>\n");
            ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");
            ToReturn+=__T("\t\t\t\t\t<!-- <ebucore:signingFormat signingPresenceFlag=\"")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SigningPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("\"/>-->\n");
            ToReturn+=__T("\t\t\t\t</bms:dataFormat>\n");
        }

        //format - ClosedCaptionsPresent
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ClosedCaptionsPresent")).empty())
        {
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t<!-- Not valid in XSD\n");
            ToReturn+=__T("\t\t\t\t<bms:dataFormat>\n");
            ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t\t<bms:captioningFormat captioningPresenceFlag=\"")+Ztring(MI.Get(Stream_Other, As11_Core_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("\" closed=\"true\"/>\n");
            else
                ToReturn+=__T("\t\t\t\t\t<bms:captioningFormat/> <!-- captioningPresenceFlag=\"")+Ztring(MI.Get(Stream_Other, As11_Core_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("\" closed=\"true\"/> -->\n");
            ToReturn+=__T("\t\t\t\t</bms:dataFormat>\n");
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t-->\n");
        }

        //format - OpenCaptionsPresent
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OpenCaptionsPresent")).empty())
        {
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t<!-- Not valid in XSD\n");
            ToReturn+=__T("\t\t\t\t<bms:dataFormat>\n");
            ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t\t<bms:captioningFormat captioningPresenceFlag=\"")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("\" closed=\"false\"/>\n");
            else
                ToReturn+=__T("\t\t\t\t\t<bms:captioningFormat/> <!-- captioningPresenceFlag=\"")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("\" closed=\"false\"/> -->\n");
            ToReturn+=__T("\t\t\t\t</bms:dataFormat>\n");
            if (Version<Version_1_2)
                ToReturn+=__T("\t\t\t\t-->\n");
        }

        //dataFormat
        for (size_t Pos=0; Pos<MI.Count_Get(Stream_Text); Pos++)
            Fims_Transform_Text(ToReturn, MI, Pos);

        //time codes
        for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); Pos++)
            if (MI.Get(Stream_Other, Pos, Other_Type)==__T("Time code"))
                Fims_Transform_TimeCode(ToReturn, MI, Pos, Version<=Version_1_2);

        //Metadata
        for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); Pos++)
            if (MI.Get(Stream_Other, Pos, Other_Type)==__T("Metadata"))
                Fims_Transform_Metadata(ToReturn, MI, Pos, Version<=Version_1_2);

        //containerFormat
        ToReturn+=__T("\t\t\t\t<bms:containerFormat");
        ToReturn+=__T(">");
        if (!MI.Get(Stream_General, 0, General_ID).empty())
            ToReturn+=__T(" <!-- containerFormatId=\"")+MI.Get(Stream_General, 0, General_ID)+__T("\" -->");
        ToReturn+=__T("\n");
        ToReturn+=__T("\t\t\t\t\t<bms:resourceID/>\n");

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - AS11ShimName
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ShimName")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"AS11ShimName\">")+MI.Get(Stream_Other, As11_Core_Pos, __T("ShimName"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - AS11ShimVersion
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ShimVersion")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"AS11ShimVersion\">")+MI.Get(Stream_Other, As11_Core_Pos, __T("ShimVersion"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - Format_Profile
        if (!MI.Get(Stream_General, 0, __T("Format_Profile")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"FormatProfile\">")+MI.Get(Stream_General, 0, __T("Format_Profile"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - Format_Settings
        if (!MI.Get(Stream_General, 0, __T("Format_Profile")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"FormatSettings\">")+MI.Get(Stream_General, 0, __T("Format_Settings"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - Encoded_Application
        if (!MI.Get(Stream_General, 0, __T("Encoded_Application")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"WritingApplication\">")+MI.Get(Stream_General, 0, __T("Encoded_Application"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat - technicalAttribute formatLabel=\"string\" - Encoded_Library
        if (!MI.Get(Stream_General, 0, __T("Encoded_Library/String")).empty())
        {
            ToReturn+=__T("\t\t\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"WritingLibrary\">")+MI.Get(Stream_General, 0, __T("Encoded_Library/String"))+__T("</bms:technicalAttribute>\n");
        }

        //format - containerFormat
        {
            ToReturn+=__T("\t\t\t\t\t<bms:containerFormat");
            if (!MI.Get(Stream_General, 0, General_Format).empty())
                ToReturn+=__T(" formatLabel=\"")+MI.Get(Stream_General, 0, General_Format)+__T("\"");
            //if (Version>=Version_1_1 && !MI.Get(Stream_General, 0, General_Format_Profile).empty())
            //    ToReturn+=__T(" containeFormatProfile=\"")+MI.Get(Stream_General, 0, General_Format_Profile)+__T("\"");
            ToReturn+=__T("/>\n");
        }

        ToReturn+=__T("\t\t\t\t</bms:containerFormat>\n");

        ToReturn+=__T("\t\t\t</bms:formatCollection>\n");
    }

    //format - technicalAttribute formatLabel=\"string\" - LineUpStart
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("LineUpStart")).empty())
    {
        ToReturn+=__T("\t\t\t<!-- Not valid in XSD\n");
        ToReturn+=__T("\t\t\t<ebucore:start typeLabel=\"LineUpStart\">\n");
        ToReturn+=__T("\t\t\t\t<ebucore:timecode>")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("LineUpStart"))+__T("</ebucore:timecode>\n");
        ToReturn+=__T("\t\t\t</ebucore:start>\n");
        ToReturn+=__T("\t\t\t-->\n");
    }

    //format - technicalAttribute formatLabel=\"string\" - IdentClockStart
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("IdentClockStart")).empty())
    {
        ToReturn+=__T("\t\t\t<!-- Not valid in XSD\n");
        ToReturn+=__T("\t\t\t<ebucore:start typeLabel=\"IdentClockStart\">\n");
        ToReturn+=__T("\t\t\t\t<ebucore:timecode>")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("IdentClockStart"))+__T("</ebucore:timecode>\n");
        ToReturn+=__T("\t\t\t</ebucore:start>\n");
        ToReturn+=__T("\t\t\t-->\n");
    }

    //format - duration
    bool durationDone=false;
    if (!MI.Get(Stream_General, 0, General_Duration).empty())
    {
        float64 DurationS=MI.Get(Stream_General, 0, General_Duration).To_float64()/1000;
        int64u DurationH=(int64u)(DurationS/60/60);
        DurationS-=DurationH*60*60;
        int64u DurationM=(int64u)(DurationS/60);
        DurationS-=DurationM*60;
        Ztring Duration;
        if (DurationH)
            Duration+=Ztring::ToZtring(DurationH)+__T('H');
        if (DurationM)
            Duration+=Ztring::ToZtring(DurationM)+__T('M');
        Duration+=Ztring::ToZtring(DurationS, 3)+__T('S');
        ToReturn+=__T("\t\t\t<bms:duration>\n");
        ToReturn+=__T("\t\t\t\t<tim:normalPlayTime>PT")+Duration+__T("</tim:normalPlayTime>\n");
        ToReturn+=__T("\t\t\t</bms:duration>\n");
        durationDone=true;
    }

    //duration
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TotalProgrammeDuration")).empty())
    {
        if (Version<Version_1_2 && durationDone)
            ToReturn+=__T("\t\t\t<!-- Not valid in XSD\n");
        ToReturn+=__T("\t\t\t<bms:duration typeLabel=\"TotalProgrammeDuration\">\n");
        ToReturn+=__T("\t\t\t\t<tim:timecode>")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TotalProgrammeDuration"))+__T("</tim:timecode>\n");
        ToReturn+=__T("\t\t\t</bms:duration>\n");
        if (Version<Version_1_2 && durationDone)
            ToReturn+=__T("\t\t\t-->\n");
    }

    //fileSize
    if (!MI.Get(Stream_General, 0, General_FileSize).empty())
        ToReturn+=__T("\t\t\t<bms:packageSize>")+MI.Get(Stream_General, 0, General_FileSize)+__T("</bms:packageSize>\n");

    if (As11_UkDpp_Pos!=(size_t)-1)
    {
        if (Version<Version_1_2)
            ToReturn+=__T("\t\t\t<!-- Not valid in XSD\n");

        //format - technicalAttribute formatLabel=\"string\" - AudioLoudnessStandard
        if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioLoudnessStandard")).empty())
        {
            ToReturn+=__T("\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"AudioLoudnessStandard\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioLoudnessStandard"))+__T("</bms:technicalAttribute>\n");
        }

        //format - technicalAttribute formatLabel=\"string\" - AudioDescriptionType
        if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioDescriptionType")).empty())
        {
            ToReturn+=__T("\t\t\t<bms:technicalAttribute formatLabel=\"string\" typeLabel=\"AudioDescriptionType\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioDescriptionType"))+__T("</bms:technicalAttribute>\n");
        }

        //format - technicalAttribute formatLabel=\"string\" - overallBitRate
        if (!MI.Get(Stream_General, 0, General_OverallBitRate).empty())
        {
            ToReturn+=__T("\t\t\t<bms:technicalAttribute formatLabel=\"integer\" typeLabel=\"OverallBitRate\" unit=\"bps\">")+MI.Get(Stream_General, 0, General_OverallBitRate)+__T("</bms:technicalAttribute>\n");
        }

        //format - technicalAttribute formatLabel=\"string\" - ProgrammeHasText
        if (!MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeHasText")).empty())
        {
            ToReturn+=__T("\t\t\t<bms:technicalAttribute formatLabel=\"boolean\" typeLabel=\"ProgrammeHasText\">")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProgrammeHasText"))==__T("Yes")?__T("true"):__T("false"))+__T("</bms:technicalAttribute>\n");
        }

        //format - technicalAttribute formatLabel=\"string\" - AudioDescriptionPresent
        if (!MI.Get(Stream_Other, As11_Core_Pos, __T("AudioDescriptionPresent")).empty())
        {
            ToReturn+=__T("\t\t\t<bms:technicalAttribute formatLabel=\"boolean\" typeLabel=\"AudioDescriptionPresent\">")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("AudioDescriptionPresent"))==__T("Yes")?__T("true"):__T("false"))+__T("</bms:technicalAttribute>\n");
        }

        if (Version<Version_1_2)
            ToReturn+=__T("\t\t\t-->\n");
    }

    //bmContentFormats
    ToReturn+=__T("\t\t</bms:bmContentFormat>\n");
    ToReturn+=__T("\t</bms:bmContentFormats>\n");

    //descriptions
    if (true)
    {
        ToReturn+=__T("\t<bms:descriptions>\n");
        ToReturn+=__T("\t\t<bms:description>\n");
        ToReturn+=__T("\t\t\t<bms:resourceID/>\n");
        ToReturn+=__T("\t\t\t<desc:bmContentDescription>\n");

        //title
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:title>")+(Version>=Version_1_2?Ztring(__T(" typeLabel=\"PROGRAMME TITLE\"")):Ztring())+MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle"))+__T("</desc:title>")+(Version<Version_1_2?Ztring(__T(" <!-- typeLabel=\"PROGRAMME TITLE\" -->")):Ztring())+__T("\n");
        }
        else if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:title>")+MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber"))+__T("</desc:title> <!-- typeLabel=\"EPISODE TITLE NUMBER\" -->\n");
        }

        //alternativeTitle
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("SeriesTitle")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:alternativeTitle typeLabel=\"SERIES TITLE\">")+MI.Get(Stream_Other, As11_Core_Pos, __T("SeriesTitle"))+__T("</desc:alternativeTitle>\n");
        }
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")).empty() && !MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:alternativeTitle typeLabel=\"EPISODE TITLE NUMBER\">")+MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber"))+__T("</desc:alternativeTitle>\n");
        }

        //description
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Synopsis")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:description typeLabel=\"SYNOPSIS\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Synopsis"))+__T("</desc:description>\n");
        }

        //ProductPlacement
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductPlacement")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:description typeLabel=\"PRODUCT PLACEMENT\">")+Ztring(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductPlacement"))==__T("Yes")?__T("true"):__T("false"))+__T("</desc:description>\n");
        }

        //ContactEmail / ContactTelephoneNumber
        if (As11_UkDpp_Pos!=(size_t)-1 && (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")).empty() || !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")).empty()))
        {
            ToReturn+=__T("\t\t\t\t<desc:contributor>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:contactDetails>\n");
            ToReturn+=__T("\t\t\t\t\t\t<desc:details>\n");
            if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")).empty())
                ToReturn+=__T("\t\t\t\t\t\t\t<desc:emailAddress>")+Fims_XML_Encode(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")))+__T("</desc:emailAddress>\n");
            if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")).empty())
                ToReturn+=__T("\t\t\t\t\t\t\t<desc:telephoneNumber>")+Fims_XML_Encode(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")))+__T("</desc:telephoneNumber>\n");
            ToReturn+=__T("\t\t\t\t\t\t</desc:details>\n");
            ToReturn+=__T("\t\t\t\t\t</desc:contactDetails>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:role typeLabel=\"contact\"/>\n");
            ToReturn+=__T("\t\t\t\t</desc:contributor>\n");
        }

        //Originator
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Originator")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:contributor>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:organisationDetails>\n");
            ToReturn+=__T("\t\t\t\t\t\t<desc:organisationName>")+Fims_XML_Encode(MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Originator")))+__T("</desc:organisationName>\n");
            ToReturn+=__T("\t\t\t\t\t</desc:organisationDetails>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:role typeLabel=\"originator\"/>\n");
            ToReturn+=__T("\t\t\t\t</desc:contributor>\n");
        }

        //Distributor
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Distributor")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:contributor>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:organisationDetails>\n");
            ToReturn+=__T("\t\t\t\t\t\t<desc:organisationName>")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Distributor"))+__T("</desc:organisationName>\n");
            ToReturn+=__T("\t\t\t\t\t</desc:organisationDetails>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:role typeLabel=\"distributor\"/>\n");
            ToReturn+=__T("\t\t\t\t</desc:contributor>\n");
        }

        //date
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("CopyrightYear")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:date>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:alternative typeLabel=\"copyrighted\" startYear=\"")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("CopyrightYear"))+__T("\"/>\n");
            ToReturn+=__T("\t\t\t\t</desc:date>\n");
        }

        //type
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Genre")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:type>\n");
            ToReturn+=__T("\t\t\t\t\t<desc:genre typeDefinition=\"")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Genre"))+__T("\"/>\n");
            ToReturn+=__T("\t\t\t\t</desc:type>\n");
        }

        //identifier
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductionNumber")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:identifier typeLabel=\"PRODUCTION NUMBER\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductionNumber"))+__T("</desc:identifier>\n");
        }
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OtherIdentifier")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:identifier typeLabel=\"INTERNAL IDENTIFIER\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OtherIdentifier"))+__T("</desc:identifier>\n");
        }
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OtherIdentifierType")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:identifier typeLabel=\"INTERNAL IDENTIFIER TYPE\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OtherIdentifierType"))+__T("</desc:identifier>\n");
        }

        //format - PrimaryAudioLanguage
        if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("PrimaryAudioLanguage")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:language typeLabel=\"PrimaryAudioLanguage\" xml:lang=\"")+MI.Get(Stream_Other, As11_Core_Pos, __T("PrimaryAudioLanguage"))+__T("\"/>\n");
        }

        //format - SecondaryAudioLanguage
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SecondaryAudioLanguage")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:language typeLabel=\"SecondaryAudioLanguage\" xml:lang=\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SecondaryAudioLanguage"))+__T("<\"/>\n");
        }

        //format - TertiaryAudioLanguage
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TertiaryAudioLanguage")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:language typeLabel=\"TertiaryAudioLanguage\" xml:lang=\">")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TertiaryAudioLanguage"))+__T("\"/>\n");
        }

        //language
        if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProgrammeTextLanguage")).empty())
        {
            ToReturn+=__T("\t\t\t\t<desc:language typeLabel=\"ProgrammeTextLanguage\" xml:lang=\"")+MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProgrammeTextLanguage"))+__T("\"/>\n");
        }

        //part
        if (As11_Segmentation_Pos!=(size_t)-1)
        {
            size_t Pos=1;
            for (;;)
            {
                Ztring Content=MI.Get(Stream_Other, As11_Segmentation_Pos, Ztring::ToZtring(Pos));
                if (Content.empty())
                    break;

                Ztring Begin=Content.SubString(Ztring(), __T(" + "));
                Ztring Duration=Content.SubString(__T(" + "), __T(" = "));

                ToReturn+=__T("\t\t\t\t<desc:part partNumber=\"")+Ztring::ToZtring(Pos)+__T("\" partTotalNumber=\"")+MI.Get(Stream_Other, As11_Segmentation_Pos, __T("PartTotal"))+__T("\">\n");
                ToReturn+=__T("\t\t\t\t\t<desc:partStartTime>\n");
                ToReturn+=__T("\t\t\t\t\t\t<tim:timecode>")+Begin+__T("</tim:timecode>\n");
                ToReturn+=__T("\t\t\t\t\t</desc:partStartTime>\n");
                ToReturn+=__T("\t\t\t\t\t<desc:partDuration>\n");
                ToReturn+=__T("\t\t\t\t\t\t<tim:timecode>")+Duration+__T("</tim:timecode>\n");
                ToReturn+=__T("\t\t\t\t\t</desc:partDuration>\n");
                ToReturn+=__T("\t\t\t\t</desc:part>\n");

                Pos++;
            }
        }

        ToReturn+=__T("\t\t\t</desc:bmContentDescription>\n");
        ToReturn+=__T("\t\t</bms:description>\n");
        ToReturn+=__T("\t</bms:descriptions>\n");
    }


    //bmContent
    ToReturn+=__T("</bms:bmContent>\n");

    //Carriage return
    ToReturn.FindAndReplace(__T("\n"), EOL, 0, Ztring_Recursive);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif

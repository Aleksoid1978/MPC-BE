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
#if defined(MEDIAINFO_EBUCORE_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_EbuCore.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include "MediaInfo/OutputHelpers.h"
#include <ctime>
#include <cmath>
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
void Add_TechnicalAttributeString(Node* _Node, Ztring _Value, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    _Node->Add_Child(string("ebucore:")+(Version>=Export_EbuCore::Version_1_6?"technicalAttributeString":"comment"), _Value.To_UTF8(), "typeLabel", _TypeLabel);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeString_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
    if (!Value.empty())
        Add_TechnicalAttributeString(_Node, Value, _TypeLabel, Version);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeString_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, Ztring().From_UTF8(FieldName));
    if (!Value.empty())
        Add_TechnicalAttributeString(_Node, Value, _TypeLabel, Version);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeBoolean(Node* _Node, Ztring _Value, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    _Node->Add_Child(string("ebucore:")+(Version>=Export_EbuCore::Version_1_6?"technicalAttributeBoolean":"comment"), string(_Value==__T("Yes")?"true":"false"), "typeLabel", _TypeLabel);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeBoolean_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
    if (!Value.empty())
        Add_TechnicalAttributeBoolean(_Node, Value, _TypeLabel, Version);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeBoolean_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, Ztring().From_UTF8(FieldName));
    if (!Value.empty())
        Add_TechnicalAttributeBoolean(_Node, Value, _TypeLabel, Version);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeInteger(Node* _Node, Ztring _Value, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max, const char* Unit=NULL)
{
    _Node->Add_Child("ebucore:"+string(Version>=Export_EbuCore::Version_1_6?"technicalAttributeInteger":"comment"), _Value.To_UTF8(), "typeLabel", _TypeLabel);
    if (Unit && Version>=Export_EbuCore::Version_1_6)
        _Node->Childs.back()->Add_Attribute("unit", Unit);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeInteger_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max, const char* Unit=NULL)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
    if (!Value.empty())
        Add_TechnicalAttributeInteger(_Node, Value, _TypeLabel, Version, Unit);
};

//---------------------------------------------------------------------------
void Add_TechnicalAttributeInteger_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, Node* _Node, string _TypeLabel, Export_EbuCore::version Version=Export_EbuCore::Version_Max, const char* Unit=NULL)
{
    if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
        return;
    const Ztring& Value=MI.Get(StreamKind, StreamPos, Ztring().From_UTF8(FieldName));
    if (!Value.empty())
        Add_TechnicalAttributeInteger(_Node, Value, _TypeLabel, Version, Unit);
};

//---------------------------------------------------------------------------
int32u EbuCore_VideoCompressionCodeCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
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

Ztring EbuCore_VideoCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
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
int32u EbuCore_AudioCompressionCodeCS_termID(MediaInfo_Internal &MI, size_t StreamPos)
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

Ztring EbuCore_AudioCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
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
Export_EbuCore::Export_EbuCore ()
{
}

//---------------------------------------------------------------------------
Export_EbuCore::~Export_EbuCore ()
{
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void EbuCore_WithFactor(Node* Parent, const string& Name, const Ztring& Rational, const Ztring& Num, const Ztring& Den)
{
    Ztring nominal, factorNumerator, factorDenominator;
    int64u num=0, den=0;
    if (!Num.empty() && !Den.empty())
    {
        size_t Dot=Num.rfind(__T('.')); //Num can be a float (e.g. in DisplayAspectRatio)
        if (Dot==string::npos)
        {
            num=Num.To_int64u();
            den=Den.To_int64u();
        }
        else
        {
            den=(int64u)float64_int64s(pow((float64)10, (int)(Num.size()-(Dot+1))));
            num=(int64u)float64_int64s(Num.To_float64()*den*Den.To_int64u());
        }
    }
    else
    {
        size_t Dot=Rational.rfind(__T('.'));
        if (Dot==string::npos)
        {
            //This is already an integer
            if (Name.empty())
                nominal=Rational;
            else
            {
                factorNumerator=Rational;
                factorDenominator.From_Number(1);
            }
        }
        else
        {
            //this is a float, converting it to num/den
            den=(int64u)float64_int64s(pow((float64)10, (int)(Rational.size()-(Dot+1))));
            num=(int64u)float64_int64s(Rational.To_float64()*den);
        }
    }
    if (num && den)
    {
        //We need to find the nearest integer and adapt factors
        float64 nom=((float64)num)/den;
        int64u nom_rounded=(int64u)float64_int64s(nom);
        int64u num2=float64_int64s(((float64)num)/(nom_rounded));
        int64u den2=float64_int64s(num/nom);
        if (!Name.empty() && num2==den2)
        {
            //This is an integer, no need of factor numbers
            nominal.From_Number(nom_rounded);
        }
        else if (!Name.empty() && ((float64)num2)/den2*nom_rounded==nom) //If we find an exact match
        {
            //We found it, we can have good integer and factor numbers
            factorNumerator.From_Number(num2);
            factorDenominator.From_Number(den2);
            nominal.From_Number(nom_rounded);
        }
        else
        {
            //Not found exact, we use integer of 1 and num/den directly
            factorNumerator.From_Number(num);
            factorDenominator.From_Number(den);
            nominal.From_Number(1);
        }
    }

    if(Name.empty())
    {
        Parent->Add_Child("ebucore:factorNumerator", factorNumerator);
        Parent->Add_Child("ebucore:factorDenominator", factorDenominator);
    }
    else
    {
        Node* Child=Parent->Add_Child(Name, (nominal.empty()?Ztring::ToZtring(Rational.To_float64(), 0):nominal));
        if (!factorNumerator.empty())
            Child->Add_Attribute("factorNumerator", factorNumerator);
        if (!factorDenominator.empty())
            Child->Add_Attribute("factorDenominator", factorDenominator);
    }
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
void EbuCore_Transform_Video(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, Export_EbuCore::version Version)
{
    size_t As11_UkDpp_Pos=(size_t)-1;
    for (size_t StreamPos_Temp=0; StreamPos_Temp<MI.Count_Get(Stream_Other); StreamPos_Temp++)
    {
        if (MI.Get(Stream_Other, StreamPos_Temp, Other_Format)==__T("AS-11 UKDPP"))
            As11_UkDpp_Pos=StreamPos_Temp;
    }

    Node* Child=Parent->Add_Child("ebucore:videoFormat");

    //if (!MI.Get(Stream_Video, StreamPos, Video_ID).empty())
    //    ToReturn+=__T(" videoFormatId=\"")+MI.Get(Stream_Video, StreamPos, Video_ID)+__T("\"");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Format, "videoFormatName");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Format_Version, "videoFormatVersionId");

    //width
    if (!MI.Get(Stream_Video, StreamPos, Video_Width).empty())
    {
        Ztring Width;
        if (!MI.Get(Stream_Video, StreamPos, Video_Width_Original).empty())
            Width=MI.Get(Stream_Video, StreamPos, Video_Width_Original);
        else
            Width=MI.Get(Stream_Video, StreamPos, Video_Width);
        Child->Add_Child("ebucore:width", Width, "unit", "pixel");
    }

    //height
    if (!MI.Get(Stream_Video, StreamPos, Video_Height).empty())
    {
        Ztring Height;
        if (!MI.Get(Stream_Video, StreamPos, Video_Height_Original).empty())
            Height=MI.Get(Stream_Video, StreamPos, Video_Height_Original);
        else
            Height=MI.Get(Stream_Video, StreamPos, Video_Height);
        Child->Add_Child("ebucore:height", Height, "unit", "pixel");
    }

    //lines
    Child->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Height_Original, "ebucore:lines");

    //frameRate
    if (!MI.Get(Stream_Video, StreamPos, Video_FrameRate).empty())
        EbuCore_WithFactor(Child, "ebucore:frameRate", MI.Get(Stream_Video, StreamPos, Video_FrameRate), MI.Get(Stream_Video, StreamPos, Video_FrameRate_Num), MI.Get(Stream_Video, StreamPos, Video_FrameRate_Den));

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

        Node* Child2=Child->Add_Child("ebucore:aspectRatio", "", "typeLabel", "display");
        EbuCore_WithFactor(Child2, string(), MI.Get(Stream_Video, StreamPos, Video_DisplayAspectRatio), factorNumerator, factorDenominator);
    }

    //videoEncoding
    //if (!MI.Get(Stream_Video, StreamPos, Video_Format_Profile).empty())
    {
        int32u TermID=EbuCore_VideoCompressionCodeCS_termID(MI, StreamPos);
        Ztring typeLabel;
        Ztring TermID_String;
        if (TermID)
        {
            typeLabel=EbuCore_VideoCompressionCodeCS_Name(TermID, MI, StreamPos);
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
        Node* Child2=Child->Add_Child("ebucore:videoEncoding", "", "typeLabel", typeLabel);
        if (!TermID_String.empty())
            Child2->Add_Attribute("typeLink", __T("http://www.ebu.ch/metadata/cs/ebu_VideoCompressionCodeCS.xml#")+TermID_String);
    }

    //codec
    if (!MI.Get(Stream_Video, StreamPos, Video_CodecID).empty() || !MI.Get(Stream_Video, StreamPos, Video_Format_Commercial_IfAny).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:codec");
        if (!MI.Get(Stream_Video, StreamPos, Video_CodecID).empty())
            Child2->Add_Child("ebucore:codecIdentifier")->Add_Child("dc:identifier", MI.Get(Stream_Video, StreamPos, Video_CodecID));
        Child2->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Format_Commercial_IfAny, "ebucore:name");
    }

    //bitRate
    Child->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_BitRate, "ebucore:bitRate");

    //bitRateMax
    Child->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_BitRate_Maximum, "ebucore:bitRateMax");

    //bitRateMode
    if (!MI.Get(Stream_Video, StreamPos, Video_BitRate_Mode).empty())
    {
        Ztring bitRateMode=MI.Get(Stream_Video, StreamPos, Video_BitRate_Mode);
        if (bitRateMode==__T("CBR"))
            bitRateMode=__T("constant");
        if (bitRateMode==__T("VBR"))
            bitRateMode=__T("variable");
        Child->Add_Child("ebucore:bitRateMode", bitRateMode);
    }

    //scanningFormat
    if (!MI.Get(Stream_Video, StreamPos, Video_ScanType).empty())
    {
        Ztring ScanType=MI.Get(Stream_Video, StreamPos, Video_ScanType);
        if (ScanType==__T("MBAFF"))
            ScanType=__T("Interlaced");
        ScanType.MakeLowerCase();
        Child->Add_Child("ebucore:scanningFormat", ScanType);
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
        Child->Add_Child("ebucore:scanningOrder", ScanOrder);
    }

    //videoTrack
    if (!MI.Get(Stream_Video, StreamPos, Video_ID).empty() || !MI.Get(Stream_Video, StreamPos, Video_Title).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:videoTrack");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_ID, "trackId");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Title, "trackName");
    }

    //flag_3D
    if (!MI.Get(Stream_Video, StreamPos, Video_MultiView_Count).empty())
        Child->Add_Child("ebucore:flag_3D", "true");

    //technicalAttributeString - ActiveFormatDescription
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, Video_ActiveFormatDescription, Child, "ActiveFormatDescription");

    //technicalAttributeString - Standard
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Standard, Child, "Standard");

    //technicalAttributeString - ColorSpace
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, Video_ColorSpace, Child, "ColorSpace");

    //technicalAttributeString - ChromaSubsampling
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, Video_ChromaSubsampling, Child, "ChromaSubsampling");

    //technicalAttributeString - colour_primaries
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, "colour_primaries", Child, "colour_primaries");

    //technicalAttributeString - transfer_characteristics
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, "transfer_characteristics", Child, "transfer_characteristics");

    //technicalAttributeString - matrix_coefficients
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, "matrix_coefficients", Child, "matrix_coefficients");

    //technicalAttributeString - colour_range
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Video, StreamPos, "colour_range", Child, "colour_range");

    //technicalAttributeString - StreamSize
    Add_TechnicalAttributeInteger_IfNotEmpty(MI, Stream_Video, StreamPos, Video_StreamSize, Child, "StreamSize", Export_EbuCore::Version_Max, Version>=Export_EbuCore::Version_1_6?"byte":NULL);

    //technicalAttributeString - BitDepth
    Add_TechnicalAttributeInteger_IfNotEmpty(MI, Stream_Video, StreamPos, Video_BitDepth, Child, "BitDepth", Export_EbuCore::Version_Max, Version>=Export_EbuCore::Version_1_6?"bit":NULL);

    //technicalAttributeString
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "FpaManufacturer", Child, "FPAManufacturer");

    //technicalAttributeString
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "FpaVersion", Child, "FPAVersion");

    //technicalAttributeBoolean - Format_Settings_CABAC
    if (MI.Get(Stream_Video, StreamPos, Video_Format)==__T("AVC"))
        Add_TechnicalAttributeBoolean_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Format_Settings_CABAC, Child, "CABAC");

    //technicalAttributeBoolean - Format_Settings_MBAFF
    if (MI.Get(Stream_Video, StreamPos, Video_Format)==__T("AVC") && !MI.Get(Stream_Video, StreamPos, Video_ScanType).empty())
        Child->Add_Child("ebucore:technicalAttributeBoolean", MI.Get(Stream_Video, StreamPos, Video_ScanType)==__T("MBAFF")?"true":"false", "typeLabel", "MBAFF");

    //technicalAttributeBoolean - FpaPass
    Add_TechnicalAttributeBoolean_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "FpaPass", Child, "MBAFF");

    //technicalAttributeString
    Child->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "VideoComments", "ebucore:comment", "typeLabel", "VideoComments");
}

//---------------------------------------------------------------------------
void EbuCore_Transform_Audio(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, Export_EbuCore::version Version)
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

    Node* Child=Parent->Add_Child("ebucore:audioFormat");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Format, "audioFormatName");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Format_Version, "audioFormatVersionId");

    //audioEncoding
    //if (!MI.Get(Stream_Audio, StreamPos, Audio_Format_Profile).empty())
    {
        int32u TermID=EbuCore_AudioCompressionCodeCS_termID(MI, StreamPos);
        Ztring typeLabel;
        Ztring TermID_String;
        if (TermID)
        {
            typeLabel=EbuCore_AudioCompressionCodeCS_Name(TermID, MI, StreamPos);
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

        Node* Child2=Child->Add_Child("ebucore:audioEncoding", "", "typeLabel", typeLabel);
        if (!TermID_String.empty())
            Child2->Add_Attribute("typeLink", __T("http://www.ebu.ch/metadata/cs/ebu_AudioCompressionCodeCS.xml#")+TermID_String);
    }

    //codec
    if (!MI.Get(Stream_Audio, StreamPos, Audio_CodecID).empty() || !MI.Get(Stream_Audio, StreamPos, Audio_Format_Commercial_IfAny).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:codec");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_CodecID).empty())
            Child2->Add_Child("ebucore:codecIdentifier")->Add_Child("dc:identifier", MI.Get(Stream_Audio, StreamPos, Audio_CodecID));
        Child2->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Format_Commercial_IfAny, "ebucore:name");
    }

    //audioTrackConfiguration
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("AudioTrackLayout")).empty())
        Child->Add_Child("ebucore:audioTrackConfiguration", "", "typeLabel", MI.Get(Stream_Other, As11_Core_Pos, __T("AudioTrackLayout")));

    //samplingRate
    Child->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_SamplingRate, "ebucore:samplingRate");

    //sampleSize
    Child->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_BitDepth, "ebucore:sampleSize");

    //bitRate
    Child->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_BitRate, "ebucore:bitRate");

    //bitRateMax
    Child->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_BitRate_Maximum, "ebucore:bitRateMax");

    //bitRateMode
    if (!MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Mode).empty())
    {
        Ztring bitRateMode=MI.Get(Stream_Audio, StreamPos, Audio_BitRate_Mode);
        if (bitRateMode==__T("CBR"))
            bitRateMode=__T("constant");
        if (bitRateMode==__T("VBR"))
            bitRateMode=__T("variable");
        Child->Add_Child("ebucore:bitRateMode", bitRateMode);
    }

    //audioTrack
    if (!MI.Get(Stream_Audio, StreamPos, Audio_ID).empty() || !MI.Get(Stream_Audio, StreamPos, Audio_Title).empty() || !MI.Get(Stream_Audio, StreamPos, Audio_Language).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:audioTrack");
        if (!MI.Get(Stream_Audio, StreamPos, Audio_ID).empty())
        {
            Ztring ID=MI.Get(Stream_Audio, StreamPos, Audio_ID);
            ID.FindAndReplace(__T(" / "), __T("_"));
            Child2->Add_Attribute("trackId", ID);
        }
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Title, "trackName");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Language, "trackLanguage");
   }

    //channels
    Child->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Channel_s_, "ebucore:channels");

    //format - technicalAttributeString - ChannelPositions
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_ChannelPositions, Child, "ChannelPositions");

    //format - technicalAttributeString - ChannelLayout
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_ChannelLayout, Child, "ChannelLayout");

    //technicalAttributeString - Format_Settings_Endianness
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Format_Settings_Endianness, Child, "Endianness");

    //technicalAttributeString - Format_Settings_Wrapping
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Format_Settings_Wrapping, Child, "Wrapping");

    //technicalAttributeString - StreamSize
    Add_TechnicalAttributeInteger_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_StreamSize, Child, "StreamSize", Export_EbuCore::Version_Max, Version>=Export_EbuCore::Version_1_6?"byte":NULL);

    //technicalAttributeString
    Child->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "AudioComments", "ebucore:comment", "typeLabel", "AudioComments");
}

//---------------------------------------------------------------------------
void EbuCore_Transform_Text(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos)
{
    Node* Child=Parent->Add_Child("ebucore:dataFormat");

    //if (!MI.Get(Stream_Text, StreamPos, Text_ID).empty())
    //    ToReturn+=__T(" dataFormatId=\"")+MI.Get(Stream_Text, StreamPos, Text_ID)+__T("\"");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Format_Version, "dataFormatVersionId");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Format, "dataFormatName");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_ID, "dataTrackId");

    //subtitlingTrack
    {
        Node* Child2=Child->Add_Child("ebucore:captioningFormat");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Format, "captioningFormatName");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_ID, "trackId");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Title, "typeLabel");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Language, "language");

        if (!MI.Get(Stream_Text, StreamPos, Text_CodecID).empty() || !MI.Get(Stream_Text, StreamPos, Text_Format_Commercial_IfAny).empty())
        {
            Node* Child3=Child->Add_Child("ebucore:codec");
            if (!MI.Get(Stream_Text, StreamPos, Text_CodecID).empty())
                Child3->Add_Child("ebucore:codecIdentifier")->Add_Child("dc:identifier", MI.Get(Stream_Text, StreamPos, Text_CodecID));
            Child3->Add_Child_IfNotEmpty(MI, Stream_Text, StreamPos, Text_Format_Commercial_IfAny, "ebucore:name");
        }
    }
}

//---------------------------------------------------------------------------
Ztring EbuCore_Duration(int64s MS)
{
    Ztring DurationString3;
    int64s HH, MM, Sec;

    //Hours
    HH=MS/1000/60/60; //h
    if (HH>0)
    {
        if (HH<10)
            DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(HH)+__T(":");
        else
            DurationString3+=Ztring::ToZtring(HH)+__T(":");
        MS-=HH*60*60*1000;
    }
    else
    {
        DurationString3+=__T("00:");
    }

    //Minutes
    MM=MS/1000/60; //mn
    if (MM>0 || HH>0)
    {
        if (MM<10)
            DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(MM)+__T(":");
        else
            DurationString3+=Ztring::ToZtring(MM)+__T(":");
        MS-=MM*60*1000;
    }
    else
    {
        DurationString3+=__T("00:");
    }

    //Seconds
    Sec=MS/1000; //s
    if (Sec>0 || MM>0 || HH>0)
    {
        if (Sec<10)
            DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(Sec)+__T(".");
        else
            DurationString3+=Ztring::ToZtring(Sec)+__T(".");
        MS-=Sec*1000;
    }
    else
    {
        DurationString3+=__T("00.");
    }

    //Milliseconds
    if (MS>0 || Sec>0 || MM>0 || HH>0)
    {
        if (MS<10)
            DurationString3+=Ztring(__T("00"))+Ztring::ToZtring(MS);
        else if (MS<100)
            DurationString3+=Ztring(__T("0"))+Ztring::ToZtring(MS);
        else
            DurationString3+=Ztring::ToZtring(MS);
    }
    else
    {
        DurationString3+=__T("000");
    }

    return DurationString3;
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_Unit(Node* Unit, const Ztring &Name)
{
    if (Name == __T("FocusPosition_ImagePlane")
        || Name == __T("FocusPosition_FrontLensVertex")
        || Name == __T("LensZoom_35mmStillCameraEquivalent")
        || Name == __T("LensZoom_ActualFocalLength"))
        Unit->Add_Attribute("unit", "meter");
    if (Name == __T("OpticalExtenderMagnification")
        || Name == __T("ElectricalExtenderMagnification")
        || Name == __T("CameraMasterBlackLevel")
        || Name == __T("CameraKneePoint")
        || Name == __T("CameraLuminanceDynamicRange"))
        Unit->Add_Attribute("unit", "percentage");
    if (Name == __T("ShutterSpeed_Angle")
        || Name == __T("HorizontalFieldOfView"))
        Unit->Add_Attribute("unit", "degree");
    if (Name == __T("ShutterSpeed_Time"))
        Unit->Add_Attribute("unit", "second");
    if (Name == __T("WhiteBalance"))
        Unit->Add_Attribute("unit", "kelvin");
    if (Name == __T("EffectiveFocaleLength")
        || Name == __T("ImagerDimension_EffectiveWidth")
        || Name == __T("ImagerDimension_EffectiveHeight"))
        Unit->Add_Attribute("unit", "millimeter");
    if (Name == __T("CameraMasterGainAdjustment"))
        Unit->Add_Attribute("unit", "dB");
    if (Name == __T("CaptureFrameRate"))
        Unit->Add_Attribute("unit", "fps");
    if (Name == __T("FocusDistance")
        || Name == __T("HyperfocalDistance")
        || Name == __T("NearFocusDistance")
        || Name == __T("FarFocusDistance")
        || Name == __T("EntrancePupilPosition"))
    {
        /*
        Ztring CookeProtocol_CalibrationType = MI.Get(Stream_Other, StreamPos, __T("CookeProtocol_CalibrationType_Values"));
        if (CookeProtocol_CalibrationType.find(__T(" / ")) == string::npos)
        {
            if (CookeProtocol_CalibrationType == __T("mm"))
                CookeProtocol_CalibrationType = __T("millimeter");
            if (CookeProtocol_CalibrationType == __T("in"))
                CookeProtocol_CalibrationType = __T("inch");
            Unit += __T(" unit=\"");
            Unit += CookeProtocol_CalibrationType;
            Unit += __T("\"");
        }
        */
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_Unit(Node* Unit, const Ztring &Name, const Ztring &Value)
{
    if (Value != __T("Infinite"))
    {
        EbuCore_Transform_AcquisitionMetadata_Unit(Unit, Name);
        return;
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_Unit(Node* Unit, const Ztring &Name, const ZtringList &Values)
{
    for (size_t Pos = 0; Pos < Values.size(); Pos++)
        if (Values[Pos] != __T("Infinite"))
        {
            EbuCore_Transform_AcquisitionMetadata_Unit(Unit, Name);
            return;
        }
}

//---------------------------------------------------------------------------
struct line
{
    Ztring Name;
    ZtringList Values;
    vector<int64u> FrameCounts;

    line()
    {
        Values.Separator_Set(0, __T(" / "));
    }
};

//---------------------------------------------------------------------------
Node* EbuCore_Transform_AcquisitionMetadata_Segment_Begin(Node* Parent, line& Line, size_t Values_Begin, size_t Values_End, int64u& Frame_Pos, float64 FrameRate, bool DoIncrement=true)
{
    Node* Child=Parent->Add_Child("ebucore:segment");
    Child->Add_Attribute("startTime", EbuCore_Duration(float64_int64s(Frame_Pos / FrameRate * 1000)));
    if (DoIncrement)
        Frame_Pos += (Values_End - Values_Begin) * Line.FrameCounts[Values_Begin];
    Child->Add_Attribute("endTime", EbuCore_Duration(float64_int64s((Frame_Pos+(DoIncrement?0:1)) / FrameRate * 1000)));
    return Child;
}

//---------------------------------------------------------------------------
Node* EbuCore_Transform_AcquisitionMetadata_Parameter_Begin(Node* Parent, line& Line)
{
    Node* Child=Parent->Add_Child("ebucore:parameter");
    Child->Add_Attribute("name", Line.Name);
    EbuCore_Transform_AcquisitionMetadata_Unit(Child, Line.Name, Line.Values[0]);
    return Child;
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_Common(Node* Cur_Node, line& Line, size_t& Values_Begin, size_t Values_End, float64 FrameRate)
{
    //if (Values_End - Values_Begin>1)
    //{
    //    ToReturn += __T(" interval=\"");
    //    ToReturn += Ztring::ToZtring(Line.FrameCounts[Values_Begin] / FrameRate, 3);
    //    ToReturn += __T("\"");
    //}
    if (Values_Begin < Values_End)
    {
        for (; Values_Begin < Values_End; Values_Begin++)
        {
            Line.Values[Values_Begin].FindAndReplace(__T(" "), Ztring(), 0, Ztring_Recursive);
            Cur_Node->Value += Line.Values[Values_Begin].To_UTF8();
            Cur_Node->Value += ' ';
        }
        Cur_Node->Value.resize(Cur_Node->Value.size() - 1);
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_SegmentThenParameter_PerParameter(Node* Parent, line& Line, size_t& Values_Begin, size_t Values_End, float64 FrameRate)
{
    Node* Child=EbuCore_Transform_AcquisitionMetadata_Parameter_Begin(Parent, Line);
    EbuCore_Transform_AcquisitionMetadata_Common(Child, Line, Values_Begin, Values_End, FrameRate);
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_ParameterThenSegment_PerParameter(Node* Parent, line& Line, size_t& Values_Begin, size_t Values_End, int64u& Frame_Pos, float64 FrameRate)
{
    Node* Child=EbuCore_Transform_AcquisitionMetadata_Segment_Begin(Parent, Line, Values_Begin, Values_End, Frame_Pos, FrameRate);
    EbuCore_Transform_AcquisitionMetadata_Common(Child, Line, Values_Begin, Values_End, FrameRate);
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_Run(Node* Parent, line& Line, float64 FrameRate, bool SegmentFirst, bool WithSegment)
{
    int64u Frame_Pos = 0; 
    
    for (size_t Values_Begin = 0; Values_Begin < Line.Values.size();)
    {
        //Looking for intervals
        size_t Values_End = Values_Begin + 1;
        for (; Values_End < Line.Values.size(); Values_End++)
            if (Line.FrameCounts[Values_End] != Line.FrameCounts[Values_Begin])
                break;

        //segment
        Node* Child=NULL;
        if (WithSegment)
            Child=EbuCore_Transform_AcquisitionMetadata_Segment_Begin(Parent, Line, Values_Begin, Values_End, Frame_Pos, FrameRate);

        //acquisitionParameter
        if (SegmentFirst)
            EbuCore_Transform_AcquisitionMetadata_SegmentThenParameter_PerParameter(WithSegment?Child:Parent, Line, Values_Begin, Values_End, FrameRate);
        else
            EbuCore_Transform_AcquisitionMetadata_ParameterThenSegment_PerParameter(WithSegment?Child:Parent, Line, Values_Begin, Values_End, Frame_Pos, FrameRate);
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_PerFrame_Multiple(Node* Parent, vector<line>& Lines, vector<size_t> Lines_Multiple, float64 FrameRate, int64u FrameCount)
{
    vector<size_t> Values_Pos;
    Values_Pos.resize(Lines_Multiple.size());
    vector<size_t> Values_Pos_Remain;
    Values_Pos_Remain.resize(Lines_Multiple.size());
    for (int64u Frame_Pos = 0; Frame_Pos < FrameCount; Frame_Pos++)
    {
        //Run
        Node* Child=EbuCore_Transform_AcquisitionMetadata_Segment_Begin(Parent, Lines[Lines_Multiple[0]], 0, 1, Frame_Pos, FrameRate, false);
        for (size_t i = 0; i < Lines_Multiple.size(); ++i)
        {
            if (!Values_Pos_Remain[i])
            {
                Values_Pos_Remain[i] = Lines[Lines_Multiple[i]].FrameCounts[Values_Pos[i]];
                Values_Pos[i]++;
            }
            Values_Pos_Remain[i]--;

            Node* Child2=EbuCore_Transform_AcquisitionMetadata_Parameter_Begin(Child, Lines[Lines_Multiple[i]]);

            Child2->Value += Lines[Lines_Multiple[i]].Values[Values_Pos[i] - 1].To_UTF8();
        }
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_parameterSegment(Node* Parent, vector<line> &Lines, vector<size_t> &Lines_All, float64 FrameRate)
{
    Node* Child=Parent->Add_Child("ebucore:parameterSegmentDataOutput");

    for (size_t i = 0; i < Lines_All.size(); ++i)
    {
        //Init
        line &Line = Lines[Lines_All[i]];

        //Run
        Node* Child2=EbuCore_Transform_AcquisitionMetadata_Parameter_Begin(Child, Line);
        EbuCore_Transform_AcquisitionMetadata_Run(Child2, Line, FrameRate, false, false);
    }

}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata_segmentParameter(Node* Parent, vector<line> &Lines, vector<size_t> &Lines_Unique, vector<size_t> &Lines_Multiple, float64 FrameRate)
{
    Node* Child=Parent->Add_Child("ebucore:segmentParameterDataOutput");

    //Single value
    if (!Lines_Unique.empty())
    {
        //Init
        int64u Frame_Pos = 0;

        //Run
        Node* Child2=EbuCore_Transform_AcquisitionMetadata_Segment_Begin(Child, Lines[Lines_Unique[0]], 0, 1, Frame_Pos, FrameRate);
        for (size_t i = 0; i < Lines_Unique.size(); ++i)
            EbuCore_Transform_AcquisitionMetadata_Run(Child2, Lines[Lines_Unique[i]], FrameRate, true, false);
    }

    //Multiple values
    for (size_t i = 0; i < Lines_Multiple.size(); ++i)
    {
        //Run
        EbuCore_Transform_AcquisitionMetadata_Run(Child, Lines[Lines_Multiple[i]], FrameRate, true, true);
    }
}

//---------------------------------------------------------------------------
void EbuCore_Transform_AcquisitionMetadata(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, Export_EbuCore::version Version, Export_EbuCore::acquisitiondataoutputmode AcquisitionDataOutputMode)
{
    Node* Child=Parent->Add_Child("ebucore:acquisitionData");
    Ztring StreamDuration=MI.Get(Stream_Other, StreamPos, Other_Duration_String3);
    if (!StreamDuration.empty())
        Child->Add_Child("ebucore:extractionDuration", StreamDuration);
    const Ztring FrameRate_String=MI.Get(Stream_Other, StreamPos, Other_FrameRate);
    const Ztring FrameRate_Num_String=MI.Get(Stream_Other, StreamPos, Other_FrameRate_Num);
    const Ztring FrameRate_Den_String=MI.Get(Stream_Other, StreamPos, Other_FrameRate_Den);
    float64 FrameRate=FrameRate_Num_String.To_float64();
    if (FrameRate)
        FrameRate/=FrameRate_Den_String.To_float64();
    else
        FrameRate=FrameRate_String.To_float64();
    if (FrameRate)
       EbuCore_WithFactor(Child, "ebucore:acquisitionFrameRate", FrameRate_String, FrameRate_Num_String, FrameRate_Den_String);
    int64u FrameCount=MI.Get(Stream_Other, StreamPos, Other_FrameCount).To_int64u();

    vector<size_t> Lines_Unique;
    vector<size_t> Lines_Multiple;
    vector<size_t> Lines_All;
    vector<line> Lines;
    size_t Count= MI.Count_Get(Stream_Other, StreamPos);
    for (size_t i = MediaInfoLib::Config.Info_Get(Stream_Other).size(); i < Count; ++i)
    {
        Ztring Name = MI.Get(Stream_Other, StreamPos, i, Info_Name);
        if (Name.size() > 7 && Name.find(__T("_Values"), 7) == Name.size() - 7)
        {
            size_t Pos = Lines.size();
            Lines.resize(Pos + 1);
            Lines[Pos].Name=Name.substr(0, Name.size() - 7);
            Lines[Pos].Values.Write(MI.Get(Stream_Other, StreamPos, i));
            ZtringList FrameCounts;
            FrameCounts.Separator_Set(0, __T(" / "));
            FrameCounts.Write(MI.Get(Stream_Other, StreamPos, i + 1));
            if (Lines[Pos].Values.empty() || Lines[Pos].Values.size() != FrameCounts.size())
            {
                Lines.resize(Pos); //Invalid, remove it
                continue;
            }
            for (size_t j = 0; j < FrameCounts.size(); j++)
                Lines[Pos].FrameCounts.push_back(FrameCounts[j].To_int64u()); // _FrameCounts is afters _Values

            if (FrameCounts.size() == 1)
                Lines_Unique.push_back(Pos);
            else
                Lines_Multiple.push_back(Pos);
            Lines_All.push_back(Pos);
        }
    }

    switch (AcquisitionDataOutputMode)
    {
        case Export_EbuCore::AcquisitionDataOutputMode_Default :
        case Export_EbuCore::AcquisitionDataOutputMode_parameterSegment :
                                                                    EbuCore_Transform_AcquisitionMetadata_parameterSegment(Child, Lines, Lines_All, FrameRate);
                                                                    break;
        case Export_EbuCore::AcquisitionDataOutputMode_segmentParameter :
                                                                    EbuCore_Transform_AcquisitionMetadata_segmentParameter(Child, Lines, Lines_Unique, Lines_Multiple, FrameRate);
                                                                    break;
        default:;
    }

    if (Version<Export_EbuCore::Version_1_8)
        Child->XmlCommentOut="(In EBUCore v1.8+ only)";
}

//---------------------------------------------------------------------------
void EbuCore_Transform_TimeCode(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, bool Is1_5)
{
    Node* Child=Parent->Add_Child("ebucore:timecodeFormat");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Other, StreamPos, Other_Format, "timecodeFormatName");

    //start
    {
        Child->Add_Child("ebucore:timecodeStart")->Add_Child("ebucore:timecode", MI.Get(Stream_Other, StreamPos, Other_TimeCode_FirstFrame));
    }

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty() || !MI.Get(Stream_Other, StreamPos, Other_Title).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:timecodeTrack");

        if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty())
        {
            Ztring ID=MI.Get(Stream_Other, StreamPos, Other_ID);
            if (MI.Get(Stream_Other, StreamPos, Other_ID).find(__T("-Material"))!=string::npos)
            {
                ID.FindAndReplace(__T("-Material"), Ztring());
                Child2->Add_Attribute("trackId", ID);
                Child2->Add_Attribute("typeLabel", "Material");
            }
            else if (MI.Get(Stream_Other, StreamPos, Other_ID).find(__T("-Source"))!=string::npos)
            {
                ID.FindAndReplace(__T("-Source"), Ztring());
                Child2->Add_Attribute("trackId", ID);
                Child2->Add_Attribute("typeLabel", "Source");
            }
            else
                Child2->Add_Attribute("trackId", ID);
        }
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Other, StreamPos, Other_Title, "trackName");
    }

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty())
        Add_TechnicalAttributeBoolean(Child, MI.Get(Stream_Other, StreamPos, __T("TimeCode_Striped")), "Stripped");

    if (Is1_5)
        Child->XmlCommentOut="(timecodeFormat not in XSD)";
}

//---------------------------------------------------------------------------
void EbuCore_Transform_Metadata(Node* Parent, MediaInfo_Internal &MI, size_t StreamPos, bool Is1_5)
{
    Node* Child=Parent->Add_Child("ebucore:metadataFormat");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_Other, StreamPos, Other_Format, "metadataFormatName");

    if (!MI.Get(Stream_Other, StreamPos, Other_ID).empty() || !MI.Get(Stream_Other, StreamPos, Other_Title).empty())
    {
        Node* Child2=Child->Add_Child("ebucore:metadataTrack");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Other, StreamPos, Other_ID, "trackId");
        Child2->Add_Attribute_IfNotEmpty(MI, Stream_Other, StreamPos, Other_Title, "trackName");
    }

    if (Is1_5)
        Child->XmlCommentOut="(metadataFormat not in XSD)";
}

//---------------------------------------------------------------------------
Ztring Export_EbuCore::Transform(MediaInfo_Internal &MI, version Version, acquisitiondataoutputmode AcquisitionDataOutputMode, format Format)
{
    //Current date/time is ISO format
    time_t Seconds=time(NULL);
    Ztring DateTime; DateTime.Date_From_Seconds_1970((int32u)Seconds);
    if (DateTime.size()>=4 && DateTime[0]==__T('U') && DateTime[1]==__T('T') && DateTime[2]==__T('C') && DateTime[3]==__T(' '))
    {
        DateTime.erase(0, 4);
        DateTime+=__T('Z');
    }
    Ztring Date=DateTime.substr(0, 10);
    Ztring Time=DateTime.substr(11);

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

    //ebuCoreMain
    Node Node_CoreMain("ebucore:ebuCoreMain");
    Node_CoreMain.Add_Attribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    if (Version==Version_1_5)
    {
        Node_CoreMain.Add_Attribute("xmlns:ebucore", "urn:ebu:metadata-schema:ebuCore_2014");
        Node_CoreMain.Add_Attribute("xmlns:xalan", "http://xml.apache.org/xalan");
        Node_CoreMain.Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
        Node_CoreMain.Add_Attribute("xsi:schemaLocation", "urn:ebu:metadata-schema:ebuCore_2014 http://www.ebu.ch/metadata/schemas/EBUCore/20140318/EBU_CORE_20140318.xsd");
        Node_CoreMain.Add_Attribute("version", "1.5");
    }
    else if (Version==Version_1_6)
    {
        Node_CoreMain.Add_Attribute("xmlns:ebucore", "urn:ebu:metadata-schema:ebuCore_2015");
        Node_CoreMain.Add_Attribute("xmlns:xalan", "http://xml.apache.org/xalan");
        Node_CoreMain.Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
        Node_CoreMain.Add_Attribute("xsi:schemaLocation", string("urn:ebu:metadata-schema:ebuCore_2015 http")+string(MediaInfoLib::Config.Https_Get()?"s":"")+"://www.ebu.ch/metadata/schemas/EBUCore/20150522/ebucore_20150522.xsd");
        Node_CoreMain.Add_Attribute("version", "1.6");
    }
    else if (Version==Version_1_8)
    {
        Node_CoreMain.Add_Attribute("xmlns:ebucore", "urn:ebu:metadata-schema:ebucore");
        Node_CoreMain.Add_Attribute("xmlns:xalan", "http://xml.apache.org/xalan");
        Node_CoreMain.Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
        Node_CoreMain.Add_Attribute("xsi:schemaLocation", string("urn:ebu:metadata-schema:ebucore http")+string(MediaInfoLib::Config.Https_Get()?"s":"")+"://www.ebu.ch/metadata/schemas/EBUCore/20171009/ebucore.xsd");
        Node_CoreMain.Add_Attribute("version", "1.8");
        Node_CoreMain.Add_Attribute("writingLibraryName", "MediaInfoLib");
        Node_CoreMain.Add_Attribute("writingLibraryVersion", MediaInfoLib::Config.Info_Version_Get().SubString(__T(" - v"), Ztring()));
    }
    Node_CoreMain.Add_Attribute("dateLastModified", Date);
    Node_CoreMain.Add_Attribute("timeLastModified", Time);

    //coreMetadata
    Node* Node_CoreMetadata=Node_CoreMain.Add_Child("ebucore:coreMetadata");

    //title
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:title", "", "typeLabel", "PROGRAMME TITLE");
        Child->Add_Child("dc:title", MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle")));
    }
    else if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:title", "", "typeLabel", "EPISODE TITLE NUMBER");
        Child->Add_Child("dc:title", MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")));
    }

    //alternativeTitle
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("SeriesTitle")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:alternativeTitle", "", "typeLabel", "SERIES TITLE");
        Child->Add_Child("dc:title", MI.Get(Stream_Other, As11_Core_Pos, __T("SeriesTitle")));
    }
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")).empty() && !MI.Get(Stream_Other, As11_Core_Pos, __T("ProgrammeTitle")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:alternativeTitle", "", "typeLabel", "EPISODE TITLE NUMBER");
        Child->Add_Child("dc:title", MI.Get(Stream_Other, As11_Core_Pos, __T("EpisodeTitleNumber")));
    }

    //description
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "Synopsis", "ebucore:description", "typeLabel", "SYNOPSIS", "dc:description");

    //ProductPlacement
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductPlacement")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:description", "", "typeLabel", "PRODUCT PLACEMENT");
        Child->Add_Child("dc:description", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ProductPlacement"))==__T("yes")?"true":"false");
    }

    //ContactEmail / ContactTelephoneNumber
    if (As11_UkDpp_Pos!=(size_t)-1 && (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")).empty() || !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")).empty()))
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:contributor");
        Node* Child2=Child->Add_Child("ebucore:contactDetails");
        Node* Child3=Child2->Add_Child("ebucore:details");
        if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")).empty())
            Child3->Add_Child("ebucore:emailAddress", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactEmail")));
        if (!MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")).empty())
            Child3->Add_Child("ebucore:telephoneNumber", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("ContactTelephoneNumber")));
        Child->Add_Child("ebucore:role", "", "typeLabel", "contact");
    }

    //Originator
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Originator")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:contributor");
        Node* Child2=Child->Add_Child("ebucore:organisationDetails");
        Child2->Add_Child("ebucore:organisationName", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Originator")));
        Child->Add_Child("ebucore:role", "", "typeLabel", "originator");
    }

    //Distributor
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Distributor")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:contributor");
        Node* Child2=Child->Add_Child("ebucore:organisationDetails");
        Child2->Add_Child("ebucore:organisationName", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Distributor")));
        Child->Add_Child("ebucore:role", "", "typeLabel", "distributor");
    }

    //date
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("CopyrightYear")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:date");
        Child->Add_Child("ebucore:copyrighted", "", "startYear", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("CopyrightYear")));
    }

    //type
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Genre")).empty())
    {
        Node* Child=Node_CoreMetadata->Add_Child("ebucore:type");
        Child->Add_Child("ebucore:genre", "", "typeDefinition", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("Genre")));
    }

    //format
    Node* Node_Format=Node_CoreMetadata->Add_Child("ebucore:format");

    //format - videoFormat
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Video); Pos++)
        EbuCore_Transform_Video(Node_Format, MI, Pos, Version);

    //format - audioFormat
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Audio); Pos++)
        EbuCore_Transform_Audio(Node_Format, MI, Pos, Version);

    //format - containerFormat
    Node* Node_Format_ContainerFormat=Node_Format->Add_Child("ebucore:containerFormat");
    Node_Format_ContainerFormat->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_Format, Version>=Version_1_6?"containerFormatName":"formatLabel");
    Node_Format_ContainerFormat->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_ID, "containerFormatId");
    if (Version >= Version_1_6)
    {
        Node* Node_Format_ContainerFormat_ContainerEncoding=Node_Format_ContainerFormat->Add_Child("ebucore:containerEncoding");
        Node_Format_ContainerFormat_ContainerEncoding->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_Format, "formatLabel");
        //if (Version>=Version_1_6 && !MI.Get(Stream_General, 0, General_Format_Profile).empty())
        //    ToReturn+=__T(" containeFormatProfile=\"")+MI.Get(Stream_General, 0, General_Format_Profile)+__T("\"");
    }
    if (!MI.Get(Stream_General, 0, General_CodecID).empty() || (!MI.Get(Stream_General, 0, General_Format_Commercial_IfAny).empty()))
    {
        Node* Child=Node_Format_ContainerFormat->Add_Child("ebucore:codec");
        if (!MI.Get(Stream_General, 0, General_CodecID).empty())
        {
            Node* Child2=Child->Add_Child("ebucore:codecIdentifier");
            Child2->Add_Child("dc:identifier", MI.Get(Stream_General, 0, General_CodecID));
        }
        if (!MI.Get(Stream_General, 0, General_Format_Commercial_IfAny).empty())
            Child->Add_Child("ebucore:name", MI.Get(Stream_General, 0, General_Format_Commercial_IfAny));
    }
    //format - containerFormat - technicalAttributeString - AS11ShimName
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_Core_Pos, "ShimName", Node_Format_ContainerFormat, "AS11ShimName", Version);

    //format - containerFormat - technicalAttributeString - AS11ShimVersion
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_Core_Pos, "ShimVersion", Node_Format_ContainerFormat, "AS11ShimVersion", Version);

    //format - containerFormat - technicalAttributeString - Format_Profile
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_General, 0, "Format_Profile", Node_Format_ContainerFormat, "FormatProfile", Version);

    //format - containerFormat - technicalAttributeString - Format_Settings
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_General, 0, "Format_Settings", Node_Format_ContainerFormat, "FormatSettings", Version);

    //format - containerFormat - technicalAttributeString - Encoded_Application
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_General, 0, "Encoded_Application", Node_Format_ContainerFormat, "WritingApplication", Version);

    //format - containerFormat - technicalAttributeString - Encoded_Library
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_General, 0, "Encoded_Library/String", Node_Format_ContainerFormat, "WritingLibrary", Version);

    //format - SigningPresent
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SigningPresent")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:signingFormat", "", "signingPresenceFlag", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("SigningPresent"))==__T("Yes")?"true":"false");
        if (Version==Version_1_5)
            Child->XmlCommentOut="(signingPresenceFlag not in XSD)";
    }

    //format - dataFormat
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Text); Pos++)
        EbuCore_Transform_Text(Node_Format, MI, Pos);

    //format - ClosedCaptionsPresent
    if (As11_Core_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_Core_Pos, __T("ClosedCaptionsPresent")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:dataFormat");
        Node* Child2=Child->Add_Child("ebucore:captioningFormat", "", "captioningPresenceFlag", MI.Get(Stream_Other, As11_Core_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?"true":"false");
        Child2->Add_Attribute("closed", "true");
    }

    //format - OpenCaptionsPresent
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("OpenCaptionsPresent")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:dataFormat");
        Node* Child2=Child->Add_Child("ebucore:captioningFormat", "", "captioningPresenceFlag", MI.Get(Stream_Other, As11_Core_Pos, __T("OpenCaptionsPresent"))==__T("Yes")?"true":"false");
        Child2->Add_Attribute("closed", "false");
    }

    //format - time codes
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); Pos++)
        if (MI.Get(Stream_Other, Pos, Other_Type)==__T("Time code"))
            EbuCore_Transform_TimeCode(Node_Format, MI, Pos, Version==Version_1_5);

    //format - Metadata
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); Pos++)
        if (MI.Get(Stream_Other, Pos, Other_Type)==__T("Metadata"))
            EbuCore_Transform_Metadata(Node_Format, MI, Pos, Version==Version_1_5);

    for (size_t Pos=0; Pos<MI.Count_Get(Stream_Other); Pos++)
        if (MI.Get(Stream_Other, Pos, Other_Format)==__T("Acquisition Metadata"))
            EbuCore_Transform_AcquisitionMetadata(Node_Format, MI, Pos, Version, AcquisitionDataOutputMode);

    //format - technicalAttributeString - LineUpStart
    bool startDone=false;
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("LineUpStart")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:start", "", "typeLabel", "LineUpStart");
        Child->Add_Child("ebucore:timecode", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("LineUpStart")));
        startDone=true;
    }

    //format - technicalAttributeString - IdentClockStart
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("IdentClockStart")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:start", "", "typeLabel", "IdentClockStart");
        if (Version==Version_1_5 && startDone)
            Child->XmlCommentOut="Not valid in XSD";
        Child->Add_Child("ebucore:timecode", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("IdentClockStart")));
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

        Node* Child=Node_Format->Add_Child("ebucore:duration");
        Child->Add_Child("ebucore:normalPlayTime", __T("PT")+Duration);
        durationDone=true;
    }

    //format - duration
    if (As11_UkDpp_Pos!=(size_t)-1 && !MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TotalProgrammeDuration")).empty())
    {
        Node* Child=Node_Format->Add_Child("ebucore:duration", "", "typeLabel", "TotalProgrammeDuration");
        if (Version==Version_1_5 && durationDone)
            Child->XmlCommentOut="Not valid in XSD";
        Child->Add_Child("ebucore:timecode", MI.Get(Stream_Other, As11_UkDpp_Pos, __T("TotalProgrammeDuration")));
    }

    //format - fileSize
    Node_Format->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_FileSize, "ebucore:fileSize");

    //format - fileName
    if (!MI.Get(Stream_General, 0, General_FileName).empty())
    {
        Ztring Name=MI.Get(Stream_General, 0, General_FileName);
        if (!MI.Get(Stream_General, 0, General_FileExtension).empty())
        {
            Name+=__T('.');
            Name+=MI.Get(Stream_General, 0, General_FileExtension);
        }
        Node_Format->Add_Child("ebucore:fileName", Name);
    }

    //format - locator
    Node_Format->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_CompleteName, "ebucore:locator");

    //format - technicalAttributeString - AudioLoudnessStandard
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "AudioLoudnessStandard", Node_Format, "AudioLoudnessStandard");

    //format - technicalAttributeString - AudioDescriptionType
    Add_TechnicalAttributeString_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "AudioDescriptionType", Node_Format, "AudioDescriptionType");

    //format - technicalAttributeString - overallBitRate
    Add_TechnicalAttributeInteger_IfNotEmpty(MI, Stream_General, 0, General_OverallBitRate, Node_Format, "OverallBitRate", Version_Max, Version>=Version_1_6?"bps":NULL);

    //format - technicalAttributeString - ProgrammeHasText
    Add_TechnicalAttributeBoolean_IfNotEmpty(MI, Stream_Other, As11_Core_Pos, "ProgrammeHasText", Node_Format, "ProgrammeHasText");

    //format - technicalAttributeString - AudioDescriptionPresent
    Add_TechnicalAttributeBoolean_IfNotEmpty(MI, Stream_Other, As11_Core_Pos, "AudioDescriptionPresent", Node_Format, "AudioDescriptionPresent");

    //format - dateCreated
    if (!MI.Get(Stream_General, 0, General_Encoded_Date).empty())
    {
        Ztring DateTime=MI.Get(Stream_General, 0, General_Encoded_Date);
        if (DateTime.size()>=4 && DateTime[0]==__T('U') && DateTime[1]==__T('T') && DateTime[2]==__T('C') && DateTime[3]==__T(' '))
        {
            DateTime.erase(0, 4);
            DateTime+=__T('Z');
        }
        Ztring Date=DateTime.substr(0, 10);
        Ztring Time=DateTime.substr(11);

        Node* Child=Node_Format->Add_Child("ebucore:dateCreated");
        Child->Add_Attribute("startDate", Date);
        Child->Add_Attribute("startTime", Time);
    }

    //format - dateModified
    if (!MI.Get(Stream_General, 0, General_Tagged_Date).empty())
    {
        Ztring DateTime=MI.Get(Stream_General, 0, General_Tagged_Date);
        if (DateTime.size()>=4 && DateTime[0]==__T('U') && DateTime[1]==__T('T') && DateTime[2]==__T('C') && DateTime[3]==__T(' '))
        {
            DateTime.erase(0, 4);
            DateTime+=__T('Z');
        }
        Ztring Date=DateTime.substr(0, 10);
        Ztring Time=DateTime.substr(11);

        Node* Child=Node_Format->Add_Child("ebucore:dateModified");
        Child->Add_Attribute("startDate", Date);
        Child->Add_Attribute("startTime", Time);
    }

    //identifier
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "ProductionNumber", "ebucore:identifier", "typeLabel", "PRODUCTION NUMBER", "dc:identifier");
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "OtherIdentifier", "ebucore:identifier", "typeLabel", "INTERNAL IDENTIFIER", "dc:identifier");
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "OtherIdentifierType", "ebucore:identifier", "typeLabel", "INTERNAL IDENTIFIER TYPE", "dc:identifier");

    //format - PrimaryAudioLanguage
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_Core_Pos, "PrimaryAudioLanguage", "ebucore:language", "typeLabel", "PrimaryAudioLanguage", "dc:language");

    //format - SecondaryAudioLanguage
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "SecondaryAudioLanguage", "ebucore:language", "typeLabel", "SecondaryAudioLanguage", "dc:language");

    //format - TertiaryAudioLanguage
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "TertiaryAudioLanguage", "ebucore:language", "typeLabel", "TertiaryAudioLanguage", "dc:language");

    //language
    Node_CoreMetadata->Add_Child_IfNotEmpty(MI, Stream_Other, As11_UkDpp_Pos, "ProgrammeTextLanguage", "ebucore:language", "typeLabel", "ProgrammeTextLanguage", "dc:language");

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

            Node* Child=Node_CoreMetadata->Add_Child("ebucore:part");
            Child->Add_Attribute("partNumber", Ztring::ToZtring(Pos));
            Child->Add_Attribute("partTotalNumber", MI.Get(Stream_Other, As11_Segmentation_Pos, __T("PartTotal")));
            Child->Add_Child("ebucore:partStartTime")->Add_Child("ebucore:timecode", Begin);
            Child->Add_Child("ebucore:partDuration")->Add_Child("ebucore:timecode", Duration);
            Pos++;
        }
    }

    Ztring ToReturn;
    if(Format==Format_JSON)
        ToReturn=Ztring().From_UTF8(To_JSON(Node_CoreMain, 0).c_str());
    else
        ToReturn=Ztring().From_UTF8(To_XML(Node_CoreMain, 0).c_str());

    //Carriage return
    ToReturn.FindAndReplace(__T("\n"), EOL, 0, Ztring_Recursive);

    return ToReturn;
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif

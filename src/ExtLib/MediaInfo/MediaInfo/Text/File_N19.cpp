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
#if defined(MEDIAINFO_N19_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Text/File_N19.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
#endif //MEDIAINFO_EVENTS
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
static const char* N19_CodePageNumber(int32u CPN)
{
    switch (CPN)
    {
        case 0x343337 : return "United States";
        case 0x383530 : return "Multilingual";
        case 0x383630 : return "Portugal";
        case 0x383633 : return "Canada-French";
        case 0x383635 : return "Nordic";
        default       : return "";
    }
}

//---------------------------------------------------------------------------
static const char* N19_CharacterCodeTable(int16u CCT)
{
    switch (CCT)
    {
        case 0x3030 : return "Latin, ISO 6937-2";
        case 0x3031 : return "Latin/Cyrillic, ISO 8859-5";
        case 0x3032 : return "Latin/Arabic, ISO 8859-6";
        case 0x3033 : return "Latin/Greek, ISO 8859-7";
        case 0x3034 : return "Latin/Hebrew, ISO 8859-8";
        default       : return "";
    }
}

//---------------------------------------------------------------------------
static string N19_CharacterCodeTable_CharSet(int16u CCT)
{
    switch (CCT)
    {
        case 0x3030 : return "ISO 6937-2";
        case 0x3031 :
        case 0x3032 :
        case 0x3033 :
        case 0x3034 : return "ISO 8859-"+to_string(CCT-0x3030+4);
        default     : ;
    }

    auto CCT0=(char)(CCT>> 8);
    auto CCT1=(char)(CCT    );
    if (CCT0>='0' && CCT0<='9' && CCT1>='0' && CCT1<='9')
    {
        string CCT_Text="CCT ";
        CCT_Text+=CCT0;
        CCT_Text+=CCT1;
        return CCT_Text;
    }

    return {};
}

//---------------------------------------------------------------------------
static int8u N19_DiskFormatCode_FrameRate_Rounded(int64u DFC64)
{
    if ((DFC64&0xFFFFFF0000FFFFFF)!=0x53544C00002E3031LL)
        return 99;
    auto DFC=(int16u)(DFC64>>24);
    auto DFC0=(int8u)(DFC>>8);
    auto DFC1=(int8u)(DFC   );
    if (DFC0<'0' || DFC0>'9' || DFC1<'0' || DFC1>'9')
        return 99;
    return (DFC0-'0')*10+DFC1-'0';
}
static float64 N19_DiskFormatCode_FrameRate_IsDropFrame(int8u DFC)
{
    switch (DFC)
    {
        case 23 :
        case 29 :
        case 47 :
        case 59 :
                    return true;
        default :   return false;
    }
}
static float64 N19_DiskFormatCode_FrameRate_FromRounded(int8u DFC)
{
    if (N19_DiskFormatCode_FrameRate_IsDropFrame(DFC))
        return DFC*((float64)24000/(float64)1001);
    else
        return DFC;
}
static float64 N19_DiskFormatCode_FrameRate(int64u DFC64)
{
    return N19_DiskFormatCode_FrameRate_FromRounded(N19_DiskFormatCode_FrameRate_Rounded(DFC64));
}

//---------------------------------------------------------------------------
static const char* N19_DisplayStandardCode(int8u DSC)
{
    switch (DSC)
    {
        case 0x30 : return "Open subtitling";
        case 0x31 : return "Level-1 teletext";
        case 0x32 : return "Level-2 teletext";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* N19_LanguageCode(int16u LC)
{
    switch (LC)
    {
        case 0x3030 : return "";
        case 0x3031 : return "sq";
        case 0x3032 : return "br";
        case 0x3033 : return "ca";
        case 0x3034 : return "hr";
        case 0x3035 : return "cy";
        case 0x3036 : return "cs";
        case 0x3037 : return "da";
        case 0x3038 : return "de";
        case 0x3039 : return "en";
        case 0x3041 : return "es";
        case 0x3042 : return "eo";
        case 0x3043 : return "et";
        case 0x3044 : return "eu";
        case 0x3045 : return "fo";
        case 0x3046 : return "fr";
        case 0x3130 : return "fy";
        case 0x3131 : return "ga";
        case 0x3132 : return "gd";
        case 0x3133 : return "gl";
        case 0x3134 : return "is";
        case 0x3135 : return "it";
        case 0x3136 : return "Lappish";
        case 0x3137 : return "la";
        case 0x3138 : return "lv";
        case 0x3139 : return "lb";
        case 0x3141 : return "lt";
        case 0x3142 : return "hu";
        case 0x3143 : return "mt";
        case 0x3144 : return "nl";
        case 0x3145 : return "no";
        case 0x3146 : return "oc";
        case 0x3230 : return "pl";
        case 0x3231 : return "pt";
        case 0x3232 : return "ro";
        case 0x3233 : return "Romansh";
        case 0x3234 : return "sr";
        case 0x3235 : return "sk";
        case 0x3236 : return "sl";
        case 0x3237 : return "fi";
        case 0x3238 : return "sv";
        case 0x3239 : return "tr";
        case 0x3241 : return "Flemish";
        case 0x3242 : return "wa";
        case 0x3435 : return "zu";
        case 0x3436 : return "vi";
        case 0x3437 : return "uz";
        case 0x3438 : return "ur";
        case 0x3439 : return "uk";
        case 0x3441 : return "th";
        case 0x3442 : return "te";
        case 0x3443 : return "tt";
        case 0x3444 : return "ta";
        case 0x3445 : return "Tadzhik";
        case 0x3446 : return "sw";
        case 0x3530 : return "Sranan Tongo";
        case 0x3531 : return "so";
        case 0x3532 : return "si";
        case 0x3533 : return "sn";
        case 0x3534 : return "sr";
        case 0x3535 : return "Ruthenian";
        case 0x3536 : return "ru";
        case 0x3537 : return "qu";
        case 0x3538 : return "ps";
        case 0x3539 : return "Punjabi";
        case 0x3541 : return "fa";
        case 0x3542 : return "Papamiento";
        case 0x3543 : return "or";
        case 0x3544 : return "ne";
        case 0x3545 : return "nr";
        case 0x3546 : return "mr";
        case 0x3630 : return "mo";
        case 0x3631 : return "ms";
        case 0x3632 : return "mg";
        case 0x3633 : return "mk";
        case 0x3634 : return "Laotian";
        case 0x3635 : return "kr";
        case 0x3636 : return "km";
        case 0x3637 : return "kk";
        case 0x3638 : return "kn";
        case 0x3639 : return "jp";
        case 0x3641 : return "id";
        case 0x3642 : return "hi";
        case 0x3643 : return "he";
        case 0x3644 : return "ha";
        case 0x3645 : return "Gurani";
        case 0x3646 : return "Gujurati";
        case 0x3730 : return "hr";
        case 0x3731 : return "ka";
        case 0x3732 : return "ff";
        case 0x3733 : return "Dari";
        case 0x3734 : return "Churash";
        case 0x3735 : return "zh";
        case 0x3736 : return "my";
        case 0x3737 : return "bg";
        case 0x3738 : return "bn";
        case 0x3739 : return "be";
        case 0x3741 : return "bm";
        case 0x3742 : return "az";
        case 0x3743 : return "as";
        case 0x3744 : return "hy";
        case 0x3745 : return "ar";
        case 0x3746 : return "am";
        default     : return "";
    }
}

//---------------------------------------------------------------------------
static string N19_YYMMDD_String(int64u V)
{
    auto V0=(int8u)(V>>40);
    auto V1=(int8u)(V>>32);
    auto V2=(int8u)(V>>24);
    auto V3=(int8u)(V>>16);
    auto V4=(int8u)(V>> 8);
    auto V5=(int8u)(V    );
    if (V0<'0' || V0>'9'
     || V1<'0' || V1>'9'
     || V2<'0' || V2>'1'
     || V3<'0' || V3>'9'
     || V4<'0' || V4>'3'
     || V5<'0' || V5>'9')
        return {};
    string ToReturn;
    if (V0<'8')
    {
        ToReturn+='2';
        ToReturn+='0';
    }
    else
    {
        ToReturn+='1';
        ToReturn+='9';
    }
    ToReturn+=V0;
    ToReturn+=V1;
    ToReturn+='-';
    ToReturn+=V2;
    ToReturn+=V3;
    ToReturn+='-';
    ToReturn+=V4;
    ToReturn+=V5;
    return ToReturn;
}

//---------------------------------------------------------------------------
static TimeCode N19_HHMMSSFF_TC(int32u V, int8u FrameRate)
{
    auto HH=(int8u)(V>>24);
    auto MM=(int8u)(V>>16);
    auto SS=(int8u)(V>> 8);
    auto FF=(int8u)(V    );
    if (MM>=60
     || SS>=61)
        return {};
    auto DropFrame=N19_DiskFormatCode_FrameRate_IsDropFrame(FrameRate);
    auto FrameMax=FrameRate;
    if (!DropFrame)
        FrameMax--;
    TimeCode TC(HH, MM, SS, FF, FrameMax, TimeCode::DropFrame(DropFrame));
    return TC;
}

//---------------------------------------------------------------------------
static TimeCode N19_HHMMSSFF_TC(int64u V, int8u FrameRate)
{
    auto V0=(int8u)(V>>56);
    auto V1=(int8u)(V>>48);
    auto V2=(int8u)(V>>40);
    auto V3=(int8u)(V>>32);
    auto V4=(int8u)(V>>24);
    auto V5=(int8u)(V>>16);
    auto V6=(int8u)(V>> 8);
    auto V7=(int8u)(V    );
    if (V0<'0' || V0>'9'
     || V1<'0' || V1>'9'
     || V2<'0' || V2>'5'
     || V3<'0' || V3>'9'
     || V4<'0' || V4>'6'
     || V5<'0' || V5>'9'
     || V6<'0' || V6>'9'
     || V7<'0' || V7>'9')
        return {};
    auto DropFrame=N19_DiskFormatCode_FrameRate_IsDropFrame(FrameRate);
    auto FrameMax=FrameRate;
    if (!DropFrame)
        FrameMax--;
    TimeCode TC((V0-'0')*10+V1-'0',
                (V2-'0')*10+V3-'0',
                (V4-'0')*10+V5-'0',
                (V6-'0')*10+V7-'0',
                FrameMax,
                TimeCode::DropFrame(DropFrame));
    return TC;
}

//---------------------------------------------------------------------------
static void CC2_Adapt(int16u& V)
{
    auto V0=(int8u)(V>>8);
    auto V1=(int8u)(V   );
    int16u Result;
    if (V0>='0' && V0<='9')
        Result=(V0-'0')*10;
    else if (V0==' ')
        Result=0;
    else
    {
        V=0;
        return;
    }

    if (V1>='0' && V1<='9')
        Result+=V1-'0';
    else if (V0!=' ')
    {
        V=0;
        return;
    }

    V=Result;
}

//---------------------------------------------------------------------------
static void CC8_Adapt(int64u& V, int8u Count)
{
    if (!Count)
    {
        V=0;
        return;
    }

    int64u Result=0;
    int8u  Offset=Count<<3;
    do
    {
        Offset-=8;
        auto V0=(int8u)(V>>Offset);
        if (V0==' ')
            break;
        if (V0<'0' || V0>'9')
        {
            V=0;
            return;
        }
        Result*=10;
        Result+=V0-'0';
    }
    while (Offset);
    V=Result;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_N19::File_N19()
:File__Analyze()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_N19;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS

    #if MEDIAINFO_DEMUX
        Row_Values=NULL;
    #endif //MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
File_N19::~File_N19()
{
    #if MEDIAINFO_DEMUX
        if (Row_Values)
        {
            for (int8u Row_Pos=0; Row_Pos<Row_Max; ++Row_Pos)
                delete[] Row_Values[Row_Pos];
            delete[] Row_Values;
        }
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_N19::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<11)
        return false; //Must wait for more data

    if (Buffer[ 3]!=0x53
     || Buffer[ 4]!=0x54   
     || Buffer[ 5]!=0x4C   
     || Buffer[ 8]!=0x2E   
     || Buffer[ 9]!=0x30   
     || Buffer[10]!=0x31) // "STLxx.01"
    {
        Reject("N19");
        return false;
    }

    //Element_Size
    if (Buffer_Size<1024)
        return false; //Must wait for more data about GSI

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_N19::FileHeader_Parse()
{
    Element_Name("General Subtitle Information");

    //Parsing
    Ztring OPT, OET, TPT, TET, TN, TCO, SLR, PUB, EN, ECD;
    int64u DFC, CD, RD, TNB, TNS, TCP, TCF;
    int32u CPN, CO;
    int16u CCT, LC, RN, MNC, MNR;
    int8u  DSC, TCS;
    Get_C3    (    CPN,                                         "CPN - Code Page Number"); Param_Info1(N19_CodePageNumber(CPN));
    Get_C8    (    DFC,                                         "DFC - Disk Format Code"); Param_Info1C(N19_DiskFormatCode_FrameRate_Rounded(DFC), N19_DiskFormatCode_FrameRate(DFC));
    Get_C1    (    DSC,                                         "DSC - Display Standard Code"); Param_Info1(N19_DisplayStandardCode(DSC));
    Get_C2    (    CCT,                                         "CCT - Character Code Table number"); Param_Info1(N19_CharacterCodeTable(CCT));
    Get_C2    (    LC ,                                         "LC - Language Code"); Param_Info1(N19_LanguageCode(LC));
    Get_Local (32, OPT,                                         "OPT - Original Programme Title");
    Get_Local (32, OET,                                         "OET - Original Episode Title");
    Get_Local (32, TPT,                                         "TPT - Translated Programme");
    Get_Local (32, TET,                                         "TET - Translated Episode");
    Get_Local (32, TN ,                                         "TN - Translator's Name");
    Get_Local (32, TCO,                                         "TCD - Translator's Contact Details");
    Get_Local (16, SLR,                                         "SLR - Subtitle List Reference Code");
    Get_C6    (    CD ,                                         "CD - Creation Date");
    Get_C6    (    RD ,                                         "RD - Revision Date");
    Get_C2    (    RN ,                                         "RN - Revision number");
    Get_C5    (    TNB,                                         "TNB - Total Number of Text and Timing Information (TTI) blocks");
    Get_C5    (    TNS,                                         "TNS - Total Number of Subtitles");
    Skip_C3   (                                                 "TNG - Total Number of Subtitle Groups");
    Get_C2    (    MNC,                                         "MNC - Maximum Number of Displayable Characters in any text row");
    Get_C2    (    MNR,                                         "MNR - Maximum Number of Displayable Rows");
    Get_C1    (    TCS,                                         "TCS - Time Code: Status");
    Get_C8    (    TCP,                                         "TCP - Time Code: Start-of-Programme");
    Get_C8    (    TCF,                                         "TCF - Time Code: First In-Cue");
    Skip_C1   (                                                 "TND - Total Number of Disks");
    Skip_C1   (                                                 "DSN - Disk Sequence Number");
    Get_C3    (    CO ,                                         "CO - Country of Origin");
    Get_Local (32, PUB,                                         "PUB - Publisher");
    Get_Local (32, EN ,                                         "EN - Editor's Name");
    Get_Local (32, ECD,                                         "ECD - Editor's Contact Details");
    Skip_XX(75,                                                 "Spare Bytes");
    Skip_XX(576,                                                "UDA - User-Defined Area");

    FILLING_BEGIN();
        FrameRate=N19_DiskFormatCode_FrameRate_Rounded(DFC);

        Accept("N19");

        Fill(Stream_General, 0, General_Format, "N19");
        Fill(Stream_General, 0, General_Title, OPT.Trim());
        if (OPT!=OET.Trim())
            Fill(Stream_General, 0, General_Title, OET);
        if (RN!=0x2030 && RN!=0x3030)
        {
            auto RN0=(char)(RN>> 8);
            auto RN1=(char)(RN);
            if ((RN0==' ' || RN0 >= '0' && RN0<'9') && RN1>='0' && RN1<'9')
            {
                string RN_Text;
                if (RN0!=' ' && RN0!='0')
                    RN_Text+=RN0;
                RN_Text+=RN1;
                Fill(Stream_General, 0, "Revision", RN_Text);
            }
        }
        Fill(Stream_General, 0, General_EncodedBy, TN.Trim());
        Fill(Stream_General, 0, General_EncodedBy, TCO.Trim());
        Fill(Stream_General, 0, General_Encoded_Date, N19_YYMMDD_String(CD));
        Fill(Stream_General, 0, General_Tagged_Date, N19_YYMMDD_String(RD));
        Fill(Stream_General, 0, General_CatalogNumber, SLR.Trim());
        Fill(Stream_General, 0, General_Country, Ztring().From_CC3(CO).MakeUpperCase());
        Fill(Stream_General, 0, General_EditedBy, EN.Trim());
        if (EN!=ECD.Trim())
            Fill(Stream_General, 0, General_EditedBy, ECD);
        Fill(Stream_General, 0, General_Publisher, PUB.Trim());
        auto CPN0=(char)(CPN>>16);
        auto CPN1=(char)(CPN>> 8);
        auto CPN2=(char)(CPN    );
        if (CPN0>='0' && CPN0<'9' && CPN1>='0' && CPN1<'9' && CPN2>='0' && CPN2<'9')
        {
            string CPN_Text="CP "+Ztring().From_CC3(CPN).To_UTF8();
            Fill(Stream_General, 0, "CharacterSet", CPN_Text.c_str());
        }

        Stream_Prepare(Stream_Text);
        Fill(Stream_Text, 0, Text_Format, "N19");
        if (FrameRate)
        {
            Fill(Stream_General, 0, General_FrameRate, N19_DiskFormatCode_FrameRate_FromRounded(FrameRate));
            Fill(Stream_Text, 0, Text_FrameRate, N19_DiskFormatCode_FrameRate_FromRounded(FrameRate));
            if (TCS==0x31)
                Fill(Stream_General, 0, General_Duration_Start, (int64s)N19_HHMMSSFF_TC(TCP, FrameRate).ToMilliseconds());
        }
        CC2_Adapt(MNC);
        CC2_Adapt(MNR);
        CC8_Adapt(TNS, 5);
        if (MNC)
            Fill(Stream_Text, 0, Text_Width, MNC);
        if (MNR)
            Fill(Stream_Text, 0, Text_Height, MNR);
        Fill(Stream_Text, 0, Text_Language, N19_LanguageCode(LC));
        Fill(Stream_Text, 0, Text_Events_Total, TNS);
        Fill(Stream_Text, 0, "CharacterSet", N19_CharacterCodeTable_CharSet(CCT));

        //Init
        TCI_FirstFrame=(int32u)-1;
        LineCount=0;
        LineCount_Max=0;
        TotalLines=0;
        Line_HasContent=false;
        auto CCT_Sav=CCT;
        CC2_Adapt(CCT);
        if (CCT || CCT_Sav==0x3030)
            CharSet=(int8u)CCT;
        else
            CharSet=(int8u)-1;
        #if MEDIAINFO_DEMUX
            TCO_Previous=(int32u)-1;
            Row_Max=(int8u)MNC;
            if ((DSC=='1' || DSC=='2') && Row_Max<23)
            {
                Row_Max=23;
                IsTeletext=true;
            }
            else
                IsTeletext=false;
            Column_Max=(int8u)MNR;
            Row_Values=new wchar_t*[Row_Max];
            for (int8u Row_Pos=0; Row_Pos<Row_Max; ++Row_Pos)
            {
                Row_Values[Row_Pos]=new wchar_t[Column_Max+1];
                Row_Values[Row_Pos][Column_Max]=__T('\0');
            }
        #endif //MEDIAINFO_DEMUX
        if (TCS!=0x31)
            Finish();
    FILLING_ELSE();
        Reject();
    FILLING_END();
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_N19::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    #if MEDIAINFO_DEMUX
        TCO_Previous=(int32u)-1;
    #endif //MEDIAINFO_DEMUX

    GoTo(0x400);
    Open_Buffer_Unsynch();
    return 1;
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_N19::Header_Parse()
{
    //Filling
    Header_Fill_Size(128);
    Header_Fill_Code(0, __T("TTI"));
}

//---------------------------------------------------------------------------
void File_N19::Data_Parse()
{
    //Parsing
    Ztring TF;
    int32u TCI, TCO;
    int8u  EBN, VP, JC;
    Skip_B1   (                                                 "SGN - Subtitle Group Number");
    Skip_B2   (                                                 "SN - Subtitle Number");
    Get_B1    (EBN,                                             "EBN - Extension Block Number");
    Skip_B1   (                                                 "CS - Cumulative Status");
    Get_B4    (TCI,                                             "TCI - Time Code In");
    auto TCI_TC=N19_HHMMSSFF_TC(TCI, FrameRate);
    Param_Info1(TCI_TC.ToString());
    Get_B4    (TCO,                                             "TCO - Time Code Out");
    auto TCO_TC=N19_HHMMSSFF_TC(TCO, FrameRate);
    Param_Info1(TCO_TC.ToString());
    Get_B1    (VP,                                              "VP - Vertical Position");
    if (VP 
#if MEDIAINFO_DEMUX
        && IsTeletext
#endif
        )
        VP--; //1-Based
    Get_B1    (JC,                                              "JC - Justification Code");
    Skip_B1   (                                                 "CF - Comment Flag");
    switch (CharSet)
    {
        case 0 :        //Latin ISO 6937-2
                        Get_ISO_6937_2(112, TF,                 "TF - Text Field");
                        break;
        case 1 :        //Latin ISO 8859-5
                        Get_ISO_8859_5(112, TF,                 "TF - Text Field");
                        break;
        default:
                        //Not yet supported, basic default
                        Get_ISO_8859_1(112, TF,                 "TF - Text Field");
    }
    //Count
    TF.TrimRight(0x8F);
    if (EBN<0xF0 || EBN==0xFF)
    {
        for (auto Value : TF)
        {
            if ((Value>=0x20 && Value<0x7F) || (wchar_t)Value>=0xA0)
                Line_HasContent=true;
            else
            {
                switch (Value)
                {
                    case 0x8A:  //EOL
                                if (Line_HasContent)
                                {
                                    LineCount++;
                                    Line_HasContent=false;
                                }
                                break;
                    default  :  ;
                }
            }
        }
        if (EBN==0xFF)
        {
            if (Line_HasContent)
            {
                LineCount++;
                Line_HasContent=false;
            }
            if (LineCount_Max<LineCount)
                LineCount_Max=LineCount;
            TotalLines+=LineCount;
            LineCount=0;
        }
    }
#if MEDIAINFO_TRACE
        for (size_t i = 0; i < TF.size(); ++i)
            switch (TF[i])
            {
                case 0x8A:  //EOL
                            TF[i] = EOL[0]; 
                            {
                                size_t j = 1;
                                while (EOL[j] != __T('\0'))
                                    TF.insert(++i, 1, EOL[j++]);
                            };
                            break;
                default:
                    if ( TF[i]< 0x20
                     || (TF[i]>=0x7F && TF[i]<0xA0))
                        TF.erase(i--, 1);
            }
        Param_Info1(TF);
    #endif //MEDIAINFO_TRACE

    FILLING_BEGIN();
        #if MEDIAINFO_DEMUX
            for (size_t i = 0; i < TF.size(); ++i)
                switch (TF[i])
                {
                    case 0x8A:  //EOL
                                TF[i] = EOL[0];
                                {
                                    size_t j = 1;
                                    while (EOL[j] != __T('\0'))
                                        TF.insert(++i, 1, EOL[j++]);
                                };
                                break;
                    default:
                        if ( TF[i]< 0x20
                         || (TF[i]>=0x7F && TF[i]<0xA0))
                            TF.erase(i--, 1);
                }

            Frame_Count_NotParsedIncluded=Frame_Count;

            int8u Row_Pos=0;
            for (; Row_Pos<Row_Max; ++Row_Pos)
                for (int8u Column_Pos=0; Column_Pos<Column_Max; ++Column_Pos)
                    Row_Values[Row_Pos][Column_Pos]=__T(' ');

            if (TCO_Previous!=(int32u)-1 && TCI!=TCO_Previous)
            {
                EVENT_BEGIN (Global, SimpleText, 0)
                    Event.DTS=((int64u)N19_HHMMSSFF_TC(TCO_Previous, FrameRate).ToMilliseconds())*1000000; // "-TCP" removed for the moment. TODO: find a way for when TCP should be removed and when it should not
                    Event.PTS=Event.DTS;
                    Event.DUR=((int64u)(N19_HHMMSSFF_TC(TCI, FrameRate)-N19_HHMMSSFF_TC(TCO_Previous, FrameRate)).ToMilliseconds())*1000000;
                    Event.Content=L"";
                    Event.Flags=0;
                    Event.MuxingMode=(int8u)-1;
                    Event.Service=(int8u)-1;
                    Event.Row_Max=Row_Max;
                    Event.Column_Max=Column_Max;
                    Event.Row_Values=Row_Values;
                    Event.Row_Attributes=NULL;
                EVENT_END   ()
            }

            ZtringList List;
            List.Separator_Set(0, EOL);
            List.Write(TF);
            Row_Pos=0;
            if (VP+List.size()>Row_Max)
            {
                if (List.size()<=Row_Max)
                    VP=Row_Max-(int8u)List.size();
                else
                {
                    VP=0;
                    List.resize(Row_Max);
                }
            }
            Row_Pos=VP;
            for (; Row_Pos<Row_Max; ++Row_Pos)
            {
                if (Row_Pos-VP>=(int8u)List.size())
                    break;
                if (List[Row_Pos-VP].size()>Column_Max)
                    List[Row_Pos-VP].resize(Column_Max);

                switch (JC)
                {
                    case 1 :    //Left-justified
                    case 0 :    //Unchanged
                                for (int8u Column_Pos=0; Column_Pos<Column_Max; ++Column_Pos)
                                {
                                    if (JC==1) //Left-justified
                                        List[Row_Pos-VP].Trim();    
                                    if (Column_Pos>=List[Row_Pos-VP].size())
                                        break;
                                    if (List[Row_Pos-VP][Column_Pos]>=0x20)
                                        Row_Values[Row_Pos][Column_Pos]=List[Row_Pos-VP][Column_Pos];
                                }
                                break;
                    case 2 :    //Centered
                                for (int8u Column_Pos=(Column_Max-(int8u)List[Row_Pos-VP].size())/2; Column_Pos<Column_Max; ++Column_Pos)
                                {
                                    List[Row_Pos-VP].Trim();    
                                    if (Column_Pos-(Column_Max-List[Row_Pos-VP].size())/2>=List[Row_Pos-VP].size())
                                        break;
                                    if (List[Row_Pos-VP][Column_Pos-(Column_Max-List[Row_Pos-VP].size())/2]>=0x20)
                                        Row_Values[Row_Pos][Column_Pos]=List[Row_Pos-VP][Column_Pos-(Column_Max-List[Row_Pos-VP].size())/2];
                                }
                                break;
                    case 3 :    //Right-justified
                                for (int8u Column_Pos=Column_Max-(int8u)List[Row_Pos-VP].size(); Column_Pos<Column_Max; ++Column_Pos)
                                {
                                    List[Row_Pos-VP].Trim();    
                                    if (Column_Pos-(Column_Max-List[Row_Pos-VP].size())>=List[Row_Pos-VP].size())
                                        break;
                                    if (List[Row_Pos-VP][Column_Pos-(Column_Max-List[Row_Pos-VP].size())]>=0x20)
                                        Row_Values[Row_Pos][Column_Pos]=List[Row_Pos-VP][Column_Pos-(Column_Max-List[Row_Pos-VP].size())];
                                }
                                break;
                    default:    ;
                }
            }

            EVENT_BEGIN (Global, SimpleText, 0)
                Event.DTS=((int64u)N19_HHMMSSFF_TC(TCI, FrameRate).ToMilliseconds())*1000000; // "-TCP" removed for the moment. TODO: find a way for when TCP should be removed and when it should not
                Event.PTS=Event.DTS;
                Event.DUR=((int64u)(N19_HHMMSSFF_TC(TCO, FrameRate)-N19_HHMMSSFF_TC(TCI, FrameRate)).ToMilliseconds())*1000000;
                Event.Content=TF.To_Unicode().c_str();
                Event.Flags=0;
                Event.MuxingMode=(int8u)-1;
                Event.Service=(int8u)-1;
                Event.Row_Max=Row_Max;
                Event.Column_Max=Column_Max;
                Event.Row_Values=Row_Values;
                Event.Row_Attributes=NULL;
            EVENT_END   ()

            Frame_Count++;
            TCO_Previous=TCO;
        #endif //MEDIAINFO_DEMUX

        if (TCI_FirstFrame==(int32u)-1)
        {
            TCI_FirstFrame=TCI;
            auto Begin=N19_HHMMSSFF_TC(TCI, FrameRate);
            Fill(Stream_Text, 0, Text_Duration_Start, (int64s)Begin.ToMilliseconds());
            Fill(Stream_Text, 0, Text_TimeCode_FirstFrame, Begin.ToString());
            Fill(Stream_Text, 0, Text_Delay, (int64s)Begin.ToMilliseconds());
            Fill(Stream_Text, 0, Text_Delay_Source, "Container");
        }
        if (File_Offset+Buffer_Offset+Element_Size+128>File_Size)
        {
            auto End=N19_HHMMSSFF_TC(TCO, FrameRate);
            Fill(Stream_Text, 0, Text_Duration, (int64s)(End-N19_HHMMSSFF_TC(TCI_FirstFrame, FrameRate)).ToMilliseconds());
            Fill(Stream_Text, 0, Text_Duration_End, (int64s)End.ToMilliseconds());
            Fill(Stream_Text, 0, Text_TimeCode_LastFrame, (End-1).ToString());
            Fill(Stream_Text, 0, Text_Lines_Count, TotalLines);
            Fill(Stream_Text, 0, Text_Lines_MaxCountPerEvent, LineCount_Max);
        }
        else if (Config->ParseSpeed<0.5)
            //Jumping
            GoToFromEnd(128, "N19");
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_N19_YES

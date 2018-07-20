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
#if defined(MEDIAINFO_DSDIFF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Dsdiff.h"
#if defined(MEDIAINFO_ID3V2_YES)
    #include "MediaInfo/Tag/File_Id3v2.h"
#endif //MEDIAINFO_ID3V2_YES
#include <vector>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const size_t DSDIFF_lsConfig_Size=5;
static const char*  DSDIFF_lsConfig[DSDIFF_lsConfig_Size]=
{
    "Front: L R",
    "",
    "",
    "Front: L C R, Side: L R",
    "Front: L C R, Side: L R, LFE",
};
static const char*  DSDIFF_lsConfig_ChannelPositions2[]=
{
    "2",
    "",
    "",
    "3/2",
    "3/2.1",
};
static const char*  DSDIFF_lsConfig_ChannelLayout[DSDIFF_lsConfig_Size] =
{
    "L R",
    "",
    "",
    "L R C Ls Rs",
    "L R C LFE Ls Rs",
};

//---------------------------------------------------------------------------
static Ztring DSDIFF_CHNL_chID(int32u chID)
{
    switch (chID)
    {
        case 0x43202020 : return __T("C");
        case 0x4D4C4654 :
        case 0x534C4654 : return __T("L");
        case 0x4D524754 :
        case 0x53524754 : return __T("R");
        case 0x4C532020 : return __T("Ls");
        case 0x52532020 : return __T("Rs");
        case 0x4C464520 : return __T("LFE");
        default : return Ztring().From_CC4(chID).Trim();
    }
}


//---------------------------------------------------------------------------
static void DSDIFF_CHNL_chID_ChannelPositions(int32u chID, int32u& Mask)
{
    switch (chID)
    {
        case 0x43202020 : Mask |= 1<<1; break; //C
        case 0x4D4C4654 :
        case 0x534C4654 : Mask |= 1<<0; break; //L
        case 0x4D524754 :
        case 0x53524754 : Mask |= 1<<2; break; //R
        case 0x4C532020 : Mask |= 1<<3; break; //Ls
        case 0x52532020 : Mask |= 1<<4; break; //Rs
        case 0x4C464520 : Mask |= 1<<5; break; //LFE
        default : Mask=(int32u)-1; ; //Unknown;
    }
}
static Ztring DSDIFF_CHNL_chID_ChannelPositions(int32u Mask)
{
    if (Mask==(int32u)-1)
        return Ztring();

    Ztring ToReturn;
    if (Mask&((1<<0)|(1<<1)|(1<<2)))
    {
        ToReturn+=__T("Front: ");
        if (Mask&(1<<0))
            ToReturn+=__T("L ");
        if (Mask&(1<<1))
            ToReturn+=__T("C ");
        if (Mask&(1<<2))
            ToReturn+=__T("R ");
        ToReturn.resize(ToReturn.size()-1);
    }
    if (Mask&((1<<3)|(1<<4)))
    {
        if (!ToReturn.empty())
            ToReturn+=__T(", ");
        ToReturn+=__T("Side: ");
        if (Mask&(1<<3))
            ToReturn+=__T("L ");
        if (Mask&(1<<4))
            ToReturn+=__T("R ");
        ToReturn.resize(ToReturn.size()-1);
    }
    if (Mask&(1<<5))
    {
        if (!ToReturn.empty())
            ToReturn+=__T(", ");
        ToReturn+=__T("LFE");
    }
    return ToReturn;
}

//---------------------------------------------------------------------------
static void DSDIFF_CHNL_chID_ChannelPositions2(int32u chID, int32u& Mask)
{
    switch (chID)
    {
        case 0x43202020 :
        case 0x4D4C4654 :
        case 0x534C4654 :
        case 0x4D524754 :
        case 0x53524754 : Mask += 1; break; //Front
        case 0x4C532020 :
        case 0x52532020 : Mask += 1<<8;  break; //Side
        case 0x4C464520 : Mask += 1<<24;  break; //LFE
        default : Mask = (int32u)-1; ; //Unknown;
    }
}
static Ztring DSDIFF_CHNL_chID_ChannelPositions2(int32u Mask)
{
    if (Mask==(int32u)-1)
        return Ztring();

    Ztring ToReturn=Ztring::ToZtring(Mask&0xFF);
    if ((Mask>>8)&0xFFFF)
    {
        ToReturn+=__T('/')+Ztring::ToZtring((Mask>>8)&0xFF);
        if ((Mask>>16)&0xFF)
            ToReturn+=__T('/')+Ztring::ToZtring((Mask>>16)&0xFF);
    }
    if ((Mask>>24)&0xFF)
        ToReturn+=__T('.')+Ztring::ToZtring((Mask>>24)&0xFF);
    return ToReturn;
}

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int64u DSD_ = 0x44534420;
    const int64u DSD__COMT = 0x434F4D54;
    const int64u DSD__DIIN = 0x4449494E;
    const int64u DSD__DIIN_DIAR = 0x44494152;
    const int64u DSD__DIIN_DITI = 0x44495449;
    const int64u DSD__DIIN_EMID = 0x454D4944;
    const int64u DSD__DIIN_MARK = 0x4D41524B;
    const int64u DSD__DSD_ = 0x44534420;
    const int64u DSD__DST_ = 0x44535420;
    const int64u DSD__DST__DSTC = 0x44535443;
    const int64u DSD__DST__DSTF = 0x44535446;
    const int64u DSD__DST__FRTE = 0x46525445;
    const int64u DSD__DSTI = 0x44535449;
    const int64u DSD__FVER = 0x46564552;
    const int64u DSD__ID3_ = 0x49443320;
    const int64u DSD__PROP = 0x50524F50;
    const int64u DSD__PROP_ABSS = 0x41425353;
    const int64u DSD__PROP_CHNL = 0x43484E4C;
    const int64u DSD__PROP_CMPR = 0x434D5052;
    const int64u DSD__PROP_FS__ = 0x46532020;
    const int64u DSD__PROP_LSCO = 0x4C53434F;
    const int64u FRM8 = 0x46524D38;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dsdiff::File_Dsdiff()
:File__Analyze()
{
    //Configuration
    ParserName="Dsdiff";
    #if MEDIAINFO_EVENTS
        //ParserIDs[0]=MediaInfo_Parser_Dsdiff;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    DataMustAlwaysBeComplete=false;
}

//---------------------------------------------------------------------------
void File_Dsdiff::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "DSDIFF");
    Stream_Prepare(Stream_Audio);
}

//---------------------------------------------------------------------------
void File_Dsdiff::Streams_Finish()
{
    int32u sampleRate=Retrieve(Stream_Audio, 0, Audio_SamplingRate).To_int32u();

    if (Retrieve(Stream_Audio, 0, Audio_Format)==__T("DSD"))
    {
        int64u DSDsoundData_Size=Retrieve(Stream_Audio, 0, Audio_StreamSize).To_int64u();
        int16u numChannels=Retrieve(Stream_Audio, 0, Audio_Channel_s_).To_int16u();
        if (DSDsoundData_Size && sampleRate && numChannels)
            Fill(Stream_Audio, 0, Audio_Duration, ((float64)DSDsoundData_Size)*8*1000/numChannels/sampleRate, 3);
    }

    for (int64u Multiplier=64; Multiplier<=512; Multiplier*=2)
    {
        int64u BaseFrequency=sampleRate/Multiplier;
        if (BaseFrequency==48000 || BaseFrequency==44100)
        {
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, __T("DSD")+Ztring::ToZtring(Multiplier));
            break;
        }
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dsdiff::FileHeader_Begin()
{
    //Must have enough buffer for having header
    if (Buffer_Size<4)
        return false; //Must wait for more data

    if (Buffer[0]!=0x46 //"FRM8"
     || Buffer[1]!=0x52
     || Buffer[2]!=0x4D
     || Buffer[3]!=0x38)
    {
        Reject();
        return false;
    }

    Accept();
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsdiff::Header_Parse()
{
    //Parsing
    int64u Size;
    int32u Name;
    Get_C4 (Name,                                               "Name");
    Get_B8 (Size,                                               "Size");

    //Top level chunks
    if (Name==Elements::FRM8)
        Get_C4 (Name,                                           "Real Name");

    //Coherency check
    if (File_Offset+Buffer_Offset+Size>File_Size)
    {
        Size=File_Size-(File_Offset+Buffer_Offset);
        if (Element_Level<=2) //Incoherencies info only at the top level chunk
            Fill(Stream_General, 0, "IsTruncated", "Yes");
    }

    //Padding
    if (Size%2)
    {
        pad=true;
        Size++;
    }
    else
        pad=false;

    Header_Fill_Code(Name, Ztring().From_CC4(Name));
    Header_Fill_Size(Element_Offset+Size);
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsdiff::Data_Parse()
{
    if (pad)
        Element_Size--;

    //Parsing
    DATA_BEGIN
    LIST(DSD_)
        ATOM_BEGIN
        ATOM(DSD__COMT)
        LIST(DSD__DIIN)
            ATOM_BEGIN
            ATOM(DSD__DIIN_DIAR)
            ATOM(DSD__DIIN_DITI)
            ATOM(DSD__DIIN_EMID)
            ATOM(DSD__DIIN_MARK)
            ATOM_END
        ATOM_PARTIAL(DSD__DSD_)
        LIST(DSD__DST_)
            ATOM_BEGIN
            ATOM(DSD__DST__DSTC)
            ATOM(DSD__DST__DSTF)
            ATOM(DSD__DST__FRTE)
            ATOM_END
        ATOM_PARTIAL(DSD__DSTI)
        ATOM(DSD__FVER)
        ATOM(DSD__ID3_)
        LIST(DSD__PROP)
            ATOM_BEGIN
            ATOM(DSD__PROP_ABSS)
            ATOM(DSD__PROP_CHNL)
            ATOM(DSD__PROP_CMPR)
            ATOM(DSD__PROP_FS__)
            ATOM(DSD__PROP_LSCO)
            ATOM_END
        ATOM_END
    DATA_END

    if (pad)
    {
        Element_Size++;
        Skip_B1(                                                "pad");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsdiff::DSD_()
{
    Element_Name("Form DSD");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__COMT()
{
    Element_Name("Comments");

    //Parsing
    int16u numComments;
    Get_B2 (numComments,                                        "numComments");
    for (int16u i=0; i<numComments; i++)
    {
        Element_Begin1("comment");
        Ztring comment;
        int32u count;
        int16u cmtType, cmtRef;
        Skip_B2(                                                "timeStampYear");
        Skip_B1(                                                "TimeStampMonth");
        Skip_B1(                                                "timeStampDay");
        Skip_B1(                                                "timeStampHour");
        Skip_B1(                                                "timeStampMinutes");
        Get_B2 (cmtType,                                        "cmtType");
        Get_B2 (cmtRef,                                         "cmtRef");
        Get_B4 (count,                                          "count");
        Get_Local(count, comment,                               "commentText");
        if (count%2)
            Skip_B1(                                            "pad");

        FILLING_BEGIN();
            switch(cmtType)
            {
                case 0 : // General
                    switch (cmtRef)
                    {
                        case 0 : Fill(Stream_General, 0, General_Comment, comment); break;
                        default:;
                    }
                    break;
                case 1 : // Channel Comment
                    Fill(Stream_General, 0, General_Comment, (cmtRef?(__T("Channel ")+Ztring::ToZtring(cmtRef)+__T(": ")):Ztring())+comment); break;
                    break;
                case 2 : // Sound Source
                    switch (cmtRef)
                    {
                        case 0 : Fill(Stream_General, 0, General_OriginalSourceForm, __T("DSD recording, ")+comment); break;
                        case 1 : Fill(Stream_General, 0, General_OriginalSourceForm, __T("Analogue recording, ")+comment); break;
                        case 2 : Fill(Stream_General, 0, General_OriginalSourceForm, __T("PCM recording, ")+comment); break;
                        default: Fill(Stream_General, 0, General_OriginalSourceForm, Ztring::ToZtring(cmtRef)+__T(", ")+comment);
                    }
                    break;
                case 3 : // File History
                    switch (cmtRef)
                    {
                        case 0 : Fill(Stream_General, 0, General_Comment, comment); break;
                        case 1 : Fill(Stream_General, 0, General_EncodedBy, comment); break;
                        case 2 : Fill(Stream_General, 0, General_Encoded_Application, comment); break;
                        case 3 : Fill(Stream_General, 0, "Time zone", comment); break;
                        case 4 : Fill(Stream_General, 0, "Revision", comment); break;
                        default: Fill(Stream_General, 0, General_Comment, Ztring::ToZtring(cmtRef)+__T(", ")+comment);
                    }
                    break;
                default: Fill(Stream_General, 0, General_OriginalSourceForm, Ztring::ToZtring(cmtType)+__T(", ")+Ztring::ToZtring(cmtRef)+__T(", ")+comment);
            }
            
        FILLING_END();

        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DIIN()
{
    Element_Name("Edited Master Information");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DIIN_DIAR()
{
    Element_Name("Artist");

    //Parsing
    Ztring artist;
    int32u count;
    Get_B4 (count,                                              "count");
    Get_Local(count, artist,                                    "artistText");
    if (count%2)
        Skip_B1(                                                "pad");

    FILLING_BEGIN_PRECISE();
        Fill(Stream_General, 0, General_Performer, artist);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DIIN_DITI()
{
    Element_Name("Title");

    //Parsing
    Ztring title;
    int32u count;
    Get_B4 (count,                                              "count");
    Get_Local(count, title,                                     "titleText");
    if (count%2)
        Skip_B1(                                                "pad");

    FILLING_BEGIN_PRECISE();
        Fill(Stream_General, 0, General_Title, title);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DIIN_EMID()
{
    Element_Name("Edited Master ID");

    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "emid");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DIIN_MARK()
{
    Element_Name("Marker");

    //Parsing
    int32u count;
    Skip_B2(                                                    "hours");
    Skip_B1(                                                    "minutes");
    Skip_B1(                                                    "seconds");
    Skip_B4(                                                    "samples");
    Skip_B4(                                                    "offset");
    Skip_B2(                                                    "markType");
    Skip_B2(                                                    "markChannel");
    Skip_B2(                                                    "TrackFlags");
    Get_B4 (count,                                              "count");
    Skip_Local(count,                                           "markerText");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DSD_()
{
    Element_Name("DSD Sound Data");

    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "DSDsoundData");

    //Filling
    Fill(Stream_Audio, 0, Audio_StreamSize, Element_TotalSize_Get()-(pad?1:0));
    Fill(Stream_Audio, 0, Audio_Format_Settings, "Big"); Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, "Big");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DST_()
{
    Element_Name("DST Sound Data");

    Fill(Stream_Audio, 0, Audio_StreamSize, Element_TotalSize_Get());
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DST__DSTC()
{
    Element_Name("DST Frame CRC Chunk");

    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "crcData");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DST__DSTF()
{
    Element_Name("DST Frame Data Chunk");

    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "DSTsoundData");

    GoTo(File_Offset+Buffer_Offset+Element_TotalSize_Get(1));
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DST__FRTE()
{
    Element_Name("DST Frame Information Chunk");

    //Parsing
    int32u numFrames;
    int16u frameRate;
    Get_B4 (numFrames,                                          "numFrames");
    Get_B2 (frameRate,                                          "frameRate");

    FILLING_BEGIN_PRECISE();
        Fill(Stream_Audio, 0, Audio_FrameRate, frameRate);
        Fill(Stream_Audio, 0, Audio_FrameCount, numFrames);
        Fill(Stream_Audio, 0, Audio_Duration, ((float32)numFrames)*1000/frameRate);

        GoTo(File_Offset+Buffer_Offset+Element_TotalSize_Get(1));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__DSTI()
{
    Element_Name("DST Sound Index Chunk");

    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "indexData");
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__FVER()
{
    Element_Name("Format");

    //Parsing
    int8u version1, version2, version3, version4;
    Get_B1 (version1,                                           "version (1)");
    Get_B1 (version2,                                           "version (2)");
    Get_B1 (version3,                                           "version (3)");
    Get_B1 (version4,                                           "version (4)");

    FILLING_BEGIN_PRECISE();
        Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(version1)+__T('.')+Ztring::ToZtring(version2)+__T('.')+Ztring::ToZtring(version3)+__T('.')+Ztring::ToZtring(version4));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__ID3_()
{
    Element_Name("ID3v2 tags");

    //Parsing
    #if defined(MEDIAINFO_ID3V2_YES)
        File_Id3v2 MI;
        Open_Buffer_Init(&MI);
        Open_Buffer_Continue(&MI);
        Finish(&MI);
        Merge(MI, Stream_General, 0, 0);
    #else //defined(MEDIAINFO_ID3V2_YES)
        Skip_XX(Element_Size,                                   "Id3v2 data");
    #endif
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP()
{
    Element_Name("Property");

    //Parsing
    int32u propType;
    Get_C4 (propType,                                           "propType");

    if (propType!=0x534E4420) //"SND"
    {
        Skip_XX(Element_TotalSize_Get(),                        "Unknown");
    }
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP_ABSS()
{
    Element_Name("Absolute Start Time");

    //Parsing
    int32u samples;
    int16u hours;
    int8u minutes, seconds;
    Get_B2 (hours,                                              "hours");
    Get_B1 (minutes,                                            "minutes");
    Get_B1 (seconds,                                            "seconds");
    Get_B4 (samples,                                            "samples");

    FILLING_BEGIN();
        string Abss;
        Abss+='0'+hours/10;
        Abss+='0'+hours%10;
        Abss+=':';
        Abss+='0'+minutes/10;
        Abss+='0'+minutes%10;
        Abss+=':';
        Abss+='0'+seconds/10;
        Abss+='0'+seconds%10;
        Abss+=':';
        int32u Ratio=1000000000;
        bool Start=false;
        while (Ratio>1)
        {
            int32u S=samples%Ratio;
            if (S || Start)
            {
                Abss+='0'+S;
                Start=true;
            }
            Ratio/=10;
        }
        Abss+='0'+samples/10;
        Abss+='0'+samples%10;
        Fill(Stream_Audio, 0, "TimeCode_FirstFrame", Abss);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP_CHNL()
{
    Element_Name("Channels");

    //Parsing
    int16u numChannels;
    vector<int32u> chIDs;
    Get_B2 (numChannels,                                        "numChannels");
    while (Element_Offset<Element_Size)
    {
        int32u chID;
        Get_C4 (chID,                                           "chID");
        chIDs.push_back(chID);
    }

    FILLING_BEGIN();
        Fill(Stream_Audio, 0, Audio_Channel_s_, numChannels);
        int32u ChannelPositions=0;
        int32u ChannelPositions2=0;
        ZtringList ChannelLayout;
        ChannelLayout.Separator_Set(0, __T(" "));
        for (size_t i=0; i<chIDs.size(); i++)
        {
            DSDIFF_CHNL_chID_ChannelPositions(chIDs[i], ChannelPositions);
            DSDIFF_CHNL_chID_ChannelPositions2(chIDs[i], ChannelPositions2);
            ChannelLayout.push_back(DSDIFF_CHNL_chID(chIDs[i]));
        }
        Ztring ChannelPositions_New=DSDIFF_CHNL_chID_ChannelPositions(ChannelPositions);
        const Ztring& ChannelPositions_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelPositions);
        if (ChannelPositions_New!=ChannelPositions_Old)
            Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelPositions_New);
        Ztring ChannelPositions2_New=DSDIFF_CHNL_chID_ChannelPositions2(ChannelPositions2);
        const Ztring& ChannelPositions2_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelPositions_String2);
        if (ChannelPositions2_New!=ChannelPositions2_Old)
            Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2_New);
        const Ztring& ChannelLayout_New=ChannelLayout.Read();
        const Ztring& ChannelLayout_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelLayout);
        if (ChannelLayout_New!=ChannelLayout_Old)
            Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelLayout_New);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP_CMPR()
{
    Element_Name("Compression Type");

    //Parsing
    int32u compressionType;
    int8u Count;
    Get_B4 (compressionType,                                    "compressionType");
    Get_B1 (Count,                                              "Count");
    Skip_Local(Count,                                           "compressionName");

    FILLING_BEGIN();
        switch (compressionType)
        {
            case 0x44534420: Fill(Stream_Audio, 0, Audio_Format, "DSD"); break;
            case 0x44535420: Fill(Stream_Audio, 0, Audio_Format, "DST"); break;
            default: Fill(Stream_Audio, 0, Audio_Format, Ztring().From_CC4(compressionType));
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP_FS__()
{
    Element_Name("Sample Rate");

    //Parsing
    int32u sampleRate;
    Get_B4 (sampleRate,                                         "sampleRate");

    FILLING_BEGIN();
        Fill(Stream_Audio, 0, Audio_SamplingRate, sampleRate);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dsdiff::DSD__PROP_LSCO()
{
    Element_Name("Loudspeaker Configuration");

    //Parsing
    int16u lsConfig;
    Get_B2 (lsConfig,                                           "lsConfig");

    FILLING_BEGIN();
        if (lsConfig<DSDIFF_lsConfig_Size)
        {
            const Ztring& ChannelPositions_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelPositions);
            Ztring ChannelPositions_New=Ztring().From_UTF8(DSDIFF_lsConfig[lsConfig]);
            if (ChannelPositions_New!=ChannelPositions_Old)
                Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelPositions_New);
            const Ztring& ChannelPositions2_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelPositions_String2);
            Ztring ChannelPositions2_New=Ztring().From_UTF8(DSDIFF_lsConfig_ChannelPositions2[lsConfig]);
            if (ChannelPositions2_New!=ChannelPositions2_Old)
                Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, ChannelPositions2_New);
            Ztring ChannelLayout_New; ChannelLayout_New.From_Local(DSDIFF_lsConfig_ChannelLayout[lsConfig]);
            const Ztring& ChannelLayout_Old=Retrieve_Const(Stream_Audio, 0, Audio_ChannelLayout);
            if (ChannelLayout_New!=ChannelLayout_Old)
                Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelLayout_New);
        }
        else if (lsConfig!=0xFFFF)
        {
            Fill(Stream_Audio, 0, Audio_ChannelPositions, lsConfig);
            Fill(Stream_Audio, 0, Audio_ChannelLayout, lsConfig);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_DSDIFF_YES


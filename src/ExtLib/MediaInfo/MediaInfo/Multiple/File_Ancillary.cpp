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
#if defined(MEDIAINFO_ANCILLARY_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Ancillary.h"
#if defined(MEDIAINFO_CDP_YES)
    #include "MediaInfo/Text/File_Cdp.h"
#endif
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#if defined(MEDIAINFO_TIMECODE_YES)
    #include "MediaInfo/Multiple/File_Gxf_TimeCode.h"
#endif
#if defined(MEDIAINFO_ARIBSTDB24B37_YES)
    #include "MediaInfo/Text/File_AribStdB24B37.h"
#endif
#if defined(MEDIAINFO_SDP_YES)
    #include "MediaInfo/Text/File_Sdp.h"
#endif
#if defined(MEDIAINFO_MXF_YES)
    #include "MediaInfo/Multiple/File_Mxf.h"
#endif
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <cstring>
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace MediaInfoLib
{
//---------------------------------------------------------------------------

//***************************************************************************
// Infos
//***************************************************************************

const char* Ancillary_DataID(int8u DataID, int8u SecondaryDataID)
{
    // TODO: check http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.1364-2-201003-I!!PDF-E.pdf
    switch (DataID)
    {
        case 0x00 : return "Undefined";
        case 0x08 :
                    switch (SecondaryDataID)
                    {
                        case 0x0C : return "MPEG-2 Recoding Information";       //SMPTE ST 353
                        default   : return "(Reserved for 8-bit applications)";
                    }
                    break;
        case 0x40 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 :                                             //SMPTE ST 305
                        case 0x02 : return "SDTI";                              //SMPTE ST 348
                        case 0x04 :                                             //SMPTE ST 427
                        case 0x05 :                                             //SMPTE ST 427
                        case 0x06 : return "Link Encryption Key";               //SMPTE ST 427
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x41 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "Payload identifier";                //SMPTE ST 352
                        case 0x05 : return "Bar Data";                          //SMPTE ST 2016
                        case 0x06 : return "Pan-Scan Information";              //SMPTE ST 2016
                        case 0x07 : return "ANSI/SCTE 104 Messages";            //SMPTE ST 2010
                        case 0x08 : return "DVB/SCTE VBI Data";                 //SMPTE ST 2031
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x43 :
                    switch (SecondaryDataID)
                    {
                        case 0x02 : return "SDP";                               //OP-47 SDP, also RDD 8
                        case 0x03 : return "Multipacket";                       //OP-47 Multipacket, also RDD 8
                        case 0x05 : return "Acquisition Metadata";              //RDD 18
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x44 :
                    switch (SecondaryDataID)
                    {
                        case 0x44 : return "ISAN or UMID";                      //SMPTE RP 223
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x45 :
                    //SMPTE 2020-1-2008
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "Audio Metadata - No association";
                        case 0x02 : return "Audio Metadata - Channels 1/2";
                        case 0x03 : return "Audio Metadata - Channels 3/4";
                        case 0x04 : return "Audio Metadata - Channels 5/6";
                        case 0x05 : return "Audio Metadata - Channels 7/8";
                        case 0x06 : return "Audio Metadata - Channels 9/10";
                        case 0x07 : return "Audio Metadata - Channels 11/12";
                        case 0x08 : return "Audio Metadata - Channels 13/14";
                        case 0x09 : return "Audio Metadata - Channels 15/16";
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x46 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "Two-Frame Marker";                  //SMPTE RP 2051
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x50 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "WSS";                               //RDD 8
                        default   : return "(Reserved)";
                    }
                    break;
        case 0x51 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "Film Transfer and Video Production Information"; //RP 215
                        default   : return "(Reserved)";
                    }
                    break;
        case 0x5F :
                    switch (SecondaryDataID&0xF0)
                    {
                        case 0xD0 : return "ARIB STD B37";                      //ARIB STD B37
                        default   : return "(Reserved)";
                    }
                    break;
        case 0x60 :
                    switch (SecondaryDataID)
                    {
                        case 0x60 : return "ATC";                               //SMPTE RP 188 / SMPTE ST 12-2
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x61 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "CDP";                               //SMPTE 334
                        case 0x02 : return "CEA-608";                           //SMPTE 334
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x62 :
                    switch (SecondaryDataID)
                    {
                        case 0x01 : return "Program description";               //SMPTE 334
                        case 0x02 : return "Data broadcast";                    //SMPTE 334
                        case 0x03 : return "VBI data";                          //SMPTE 334
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x64 :
                    switch (SecondaryDataID)
                    {
                        case 0x64 : return "LTC";                               //SMPTE RP 196
                        case 0x6F : return "VITC";                              //SMPTE RP 196
                        default   : return "(Internationally registered)";
                    }
                    break;
        case 0x80 : return "Marked for deletion";
        case 0x84 : return "Data end marker";
        case 0x88 : return "Data start marker";
        default   :
                     if (DataID<=0x03)
                    return "(Reserved)";
                else if (DataID<=0x0F)
                    return "(Reserved for 8-bit applications)";
                else if (DataID<=0x3F)
                    return "(Reserved)";
                else if (DataID<=0x4F)
                    return "(Internationally registered)";
                else if (DataID<=0x5F)
                    return "(Reserved)";
                else if (DataID<=0x7F)
                    return "(Internationally registered)";
                else if (DataID<=0x83)
                    return "(Reserved)";
                else if (DataID<=0x87)
                    return "(Reserved)";
                else if (DataID<=0x9F)
                    return "(Reserved)";
                else if (DataID<=0xBF)
                    return "(Internationally registered)";
                else if (DataID<=0xCF)
                    return "User application";
                else
                    return "(Internationally registered)";
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ancillary::File_Ancillary()
:File__Analyze()
{
    //Configuration
    ParserName=__T("Ancillary");
    PTS_DTS_Needed=true;

    //In
    WithTenBit=false;
    WithChecksum=false;
    HasBFrames=false;
    InDecodingOrder=false;
    LineNumber_IsSecondField=false;
    AspectRatio=0;
    FrameRate=0;
    LineNumber=(int32u)-1;
    #if defined(MEDIAINFO_CDP_YES)
        Cdp_Parser=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)
    #if defined(MEDIAINFO_ARIBSTDB24B37_YES)
        AribStdB34B37_Parser=NULL;
    #endif //defined(MEDIAINFO_ARIBSTDB24B37_YES)
    #if defined(MEDIAINFO_SDP_YES)
        Sdp_Parser=NULL;
    #endif //defined(MEDIAINFO_SDP_YES)
    #if defined(MEDIAINFO_MXF_YES)
        Rdd18_Parser=NULL;
    #endif //defined(MEDIAINFO_MXF_YES)
}

//---------------------------------------------------------------------------
File_Ancillary::~File_Ancillary()
{
    #if defined(MEDIAINFO_CDP_YES)
        delete Cdp_Parser; //Cdp_Parser=NULL;
        for (size_t Pos=0; Pos<Cdp_Data.size(); Pos++)
            delete Cdp_Data[Pos]; //Cdp_Data[Pos]=NULL;
    #endif //defined(MEDIAINFO_CDP_YES)
    #if defined(MEDIAINFO_AFDBARDATA_YES)
        for (size_t Pos=0; Pos<AfdBarData_Data.size(); Pos++)
            delete AfdBarData_Data[Pos]; //AfdBarData_Data[Pos]=NULL;
    #endif //defined(MEDIAINFO_AFDBARDATA_YES)
    #if defined(MEDIAINFO_ARIBSTDB24B37_YES)
        delete AribStdB34B37_Parser; //AribStdB34B37_Parser=NULL;
    #endif //defined(MEDIAINFO_ARIBSTDB24B37_YES)
    #if defined(MEDIAINFO_SDP_YES)
        delete Sdp_Parser; //Sdp_Parser=NULL;
    #endif //defined(MEDIAINFO_SDP_YES)
    #if defined(MEDIAINFO_MXF_YES)
        delete Rdd18_Parser; //Rdd18_Parser=NULL;
    #endif //defined(MEDIAINFO_MXF_YES)
}

//---------------------------------------------------------------------------
void File_Ancillary::Streams_Finish()
{
    Clear();
    Stream_Prepare(Stream_General);
    Fill(Stream_General, 0, General_Format, "Ancillary");

    #if defined(MEDIAINFO_CDP_YES)
        if (Cdp_Parser && !Cdp_Parser->Status[IsFinished] && Cdp_Parser->Status[IsAccepted])
        {
            size_t StreamPos_Base=Count_Get(Stream_Text);
            Finish(Cdp_Parser);
            for (size_t StreamPos=0; StreamPos<Cdp_Parser->Count_Get(Stream_Text); StreamPos++)
            {
                Merge(*Cdp_Parser, Stream_Text, StreamPos, StreamPos_Base+StreamPos);
                Ztring MuxingMode=Cdp_Parser->Retrieve(Stream_Text, StreamPos, "MuxingMode");
                Fill(Stream_Text, StreamPos_Last, "MuxingMode", __T("Ancillary data / ")+MuxingMode, true);
            }

            Ztring LawRating=Cdp_Parser->Retrieve(Stream_General, 0, General_LawRating);
            if (!LawRating.empty())
                Fill(Stream_General, 0, General_LawRating, LawRating, true);
            Ztring Title=Cdp_Parser->Retrieve(Stream_General, 0, General_Title);
            if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                Fill(Stream_General, 0, General_Title, Title);
        }
    #endif //defined(MEDIAINFO_CDP_YES)

    #if defined(MEDIAINFO_ARIBSTDB24B37_YES)
        if (AribStdB34B37_Parser && !AribStdB34B37_Parser->Status[IsFinished] && AribStdB34B37_Parser->Status[IsAccepted])
        {
            size_t StreamPos_Base=Count_Get(Stream_Text);
            Finish(AribStdB34B37_Parser);
            for (size_t StreamPos=0; StreamPos<AribStdB34B37_Parser->Count_Get(Stream_Text); StreamPos++)
            {
                Merge(*AribStdB34B37_Parser, Stream_Text, StreamPos, StreamPos_Base+StreamPos);
                Ztring MuxingMode=AribStdB34B37_Parser->Retrieve(Stream_Text, StreamPos, "MuxingMode");
                Fill(Stream_Text,StreamPos_Last, "MuxingMode", __T("Ancillary data / ")+MuxingMode, true);
            }
        }
    #endif //defined(MEDIAINFO_ARIBSTDB24B37_YES)

    #if defined(MEDIAINFO_SDP_YES)
        if (Sdp_Parser && !Sdp_Parser->Status[IsFinished] && Sdp_Parser->Status[IsAccepted])
        {
            size_t StreamPos_Base=Count_Get(Stream_Text);
            Finish(Sdp_Parser);
            for (size_t StreamPos=0; StreamPos<Sdp_Parser->Count_Get(Stream_Text); StreamPos++)
            {
                Merge(*Sdp_Parser, Stream_Text, StreamPos, StreamPos_Base+StreamPos);
                Ztring MuxingMode=Sdp_Parser->Retrieve(Stream_General, 0, General_Format);
                Fill(Stream_Text, StreamPos_Last, "MuxingMode", __T("Ancillary data / OP-47 / ")+MuxingMode, true);
            }
        }
    #endif //defined(MEDIAINFO_SDP_YES)

    #if defined(MEDIAINFO_MXF_YES)
        if (Rdd18_Parser && !Rdd18_Parser->Status[IsFinished] && Rdd18_Parser->Status[IsAccepted])
        {
            size_t StreamPos_Base=Count_Get(Stream_Other);
            Finish(Rdd18_Parser);
            for (size_t StreamPos=0; StreamPos<Rdd18_Parser->Count_Get(Stream_Other); StreamPos++)
            {
                Merge(*Rdd18_Parser, Stream_Other, StreamPos, StreamPos_Base+StreamPos);
                Fill(Stream_Other, StreamPos_Last, Other_Format, "Acquisition Metadata", Unlimited, true, true);
                Fill(Stream_Other, StreamPos_Last, Other_MuxingMode, "Ancillary data / RDD 18");
            }
        }
    #endif //defined(MEDIAINFO_MXF_YES)

    //Unsupported streams
    for (DataID = 0; DataID<Unknown.size(); DataID++)
        for (SecondaryDataID = 0; SecondaryDataID<Unknown[DataID].size(); SecondaryDataID++)
            for (perid::iterator Stream = Unknown[DataID][SecondaryDataID].begin(); Stream!=Unknown[DataID][SecondaryDataID].end(); Stream++)
            {
                Stream_Prepare(Stream->second.StreamKind);
                for (std::map<string, Ztring>::iterator Info=Stream->second.Infos.begin(); Info!=Stream->second.Infos.end(); Info++)
                    Fill(Stream->second.StreamKind, StreamPos_Last, Info->first.c_str(), Info->second);
            }
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ancillary::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+6<=Buffer_Size && ( Buffer[Buffer_Offset  ]!=0x00
                                         ||  Buffer[Buffer_Offset+ 1]!=0xFF
                                         ||  Buffer[Buffer_Offset+ 2]!=0xFF))
        Buffer_Offset++;

    //Parsing last bytes if needed
    if (Buffer_Offset+6>Buffer_Size)
    {
        if (Buffer_Offset+5==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x00FFFF)
            Buffer_Offset++;
        if (Buffer_Offset+4==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x00FFFF)
            Buffer_Offset++;
        if (Buffer_Offset+3==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x00FFFF)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x00FF)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x00)
            Buffer_Offset++;
        return false;
    }

    if (!Status[IsAccepted])
    {
        Accept();
    }

    //Synched is OK
    return true;
}

//---------------------------------------------------------------------------
bool File_Ancillary::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+6>Buffer_Size)
        return false;

    //Quick test of synchro
    if (CC3(Buffer+Buffer_Offset)!=0x00FFFF)
    {
        Synched=false;
        if (IsSub)
            Buffer_Offset=Buffer_Size; // We don't trust the rest of the stream, we never saw real data when sync is lost and there are lot of false-positives if we sync only on 0x00FFFF
    }

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ancillary::Read_Buffer_Continue()
{
    if (Element_Size==0)
    {
        if (!Cdp_Data.empty() && AspectRatio && FrameRate)
        {
            ((File_Cdp*)Cdp_Parser)->AspectRatio=AspectRatio;
            for (size_t Pos=0; Pos<Cdp_Data.size(); Pos++)
            {
                if (Cdp_Parser->PTS_DTS_Needed)
                    Cdp_Parser->FrameInfo.DTS=FrameInfo.DTS-(Cdp_Data.size()-Pos)*FrameInfo.DUR;
                Open_Buffer_Continue(Cdp_Parser, Cdp_Data[Pos]->Data, Cdp_Data[Pos]->Size);
                delete Cdp_Data[Pos]; //Cdp_Data[0]=NULL;
            }
            Cdp_Data.clear();
        }

        #if defined(MEDIAINFO_AFDBARDATA_YES)
            //Keeping only one, TODO: parse it without video stream
            for (size_t Pos=1; Pos<AfdBarData_Data.size(); Pos++)
                delete AfdBarData_Data[Pos]; //AfdBarData_Data[0]=NULL;
            if (!AfdBarData_Data.empty())
                AfdBarData_Data.resize(1);
        #endif //defined(MEDIAINFO_AFDBARDATA_YES)

        return;
    }

    if (!Status[IsAccepted] && !MustSynchronize)
        Accept();
}

//---------------------------------------------------------------------------
void File_Ancillary::Read_Buffer_AfterParsing()
{
    Buffer_Offset=Buffer_Size; //This is per frame

    Frame_Count++;
    Frame_Count_InThisBlock++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;
}

//---------------------------------------------------------------------------
void File_Ancillary::Read_Buffer_Unsynched()
{
    #if defined(MEDIAINFO_CDP_YES)
        for (size_t Pos=0; Pos<Cdp_Data.size(); Pos++)
            delete Cdp_Data[Pos]; //Cdp_Data[Pos]=NULL;
        Cdp_Data.clear();
        if (Cdp_Parser)
            Cdp_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_CDP_YES)
    #if defined(MEDIAINFO_AFDBARDATA_YES)
        for (size_t Pos=0; Pos<AfdBarData_Data.size(); Pos++)
            delete AfdBarData_Data[Pos]; //AfdBarData_Data[Pos]=NULL;
        AfdBarData_Data.clear();
    #endif //defined(MEDIAINFO_AFDBARDATA_YES)
    #if defined(MEDIAINFO_ARIBSTDB24B37_YES)
        if (AribStdB34B37_Parser)
            AribStdB34B37_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_ARIBSTDB24B37_YES)
    #if defined(MEDIAINFO_SDP_YES)
        if (Sdp_Parser)
            Sdp_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_SDP_YES)
    #if defined(MEDIAINFO_MXF_YES)
        if (Rdd18_Parser)
            Rdd18_Parser->Open_Buffer_Unsynch();
    #endif //defined(MEDIAINFO_MXF_YES)
    AspectRatio=0;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ancillary::Header_Parse()
{
    //Parsing
    if (MustSynchronize)
    {
        if (WithTenBit)
        {
            Skip_L2(                                            "Ancillary data flag");
            Skip_L2(                                            "Ancillary data flag");
            Skip_L2(                                            "Ancillary data flag");
        }
        else
        {
            Skip_L1(                                            "Ancillary data flag");
            Skip_L1(                                            "Ancillary data flag");
            Skip_L1(                                            "Ancillary data flag");
        }
    }
    Get_L1 (DataID,                                             "Data ID");
    if (WithTenBit)
        Skip_L1(                                                "Parity+Unused"); //even:1, odd:2
    Get_L1 (SecondaryDataID,                                    "Secondary Data ID"); Param_Info1(Ancillary_DataID(DataID, SecondaryDataID));
    if (WithTenBit)
        Skip_L1(                                                "Parity+Unused"); //even:1, odd:2
    Get_L1 (DataCount,                                          "Data count");
    if (WithTenBit)
        Skip_L1(                                                "Parity+Unused"); //even:1, odd:2

    //Test (in some container formats, Cheksum is present sometimes)
    bool WithChecksum_Temp=WithChecksum;
    if (!MustSynchronize && !WithChecksum && (size_t)((3+DataCount+1)*(WithTenBit?2:1))==Buffer_Size)
        WithChecksum_Temp=true;

    //Filling
    Header_Fill_Code((((int16u)DataID)<<8)|SecondaryDataID, Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID));
    Header_Fill_Size(((MustSynchronize?3:0)+3+DataCount+(WithChecksum_Temp?1:0))*(WithTenBit?2:1));
}

//---------------------------------------------------------------------------
void File_Ancillary::Data_Parse()
{
    Element_Info1(Ancillary_DataID(DataID, SecondaryDataID));

    //Buffer
    int8u* Payload=new int8u[DataCount];
    Element_Begin1("Raw data");
    for(int8u Pos=0; Pos<DataCount; Pos++)
    {
        Get_L1 (Payload[Pos],                                   "Data");
        if (WithTenBit)
            Skip_L1(                                            "Parity+Unused"); //even:1, odd:2
    }

    //Parsing
    if (WithChecksum)
        Skip_L1(                                                "Checksum");
    if (WithTenBit)
        Skip_L1(                                                "Parity+Unused"); //even:1, odd:2
    Element_End0();

    FILLING_BEGIN();
        switch (DataID)
        {
            case 0x08 :
                        switch (SecondaryDataID)
                        {
                            case 0x0C : // (from SMPTE ST 353)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="MPEG-2 Recoding Information";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 353";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x40 :
                        switch (SecondaryDataID)
                        {
                            case 0x01 : // (from SMPTE ST 305)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="SDTI";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 305";
                                        }
                                        break;
                            case 0x02 : // (from SMPTE ST 348)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="SDTI";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 348";
                                        }
                                        break;
                            case 0x04 :
                            case 0x05 :
                            case 0x06 : // (from SMPTE ST 427)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Link Encryption Key";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 427";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x41 :
                        switch (SecondaryDataID)
                        {
                            case 0x01 : //SMPTE ST 352
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Payload identifier";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 352";
                                        }
                                        break;
                            case 0x05 : //Bar Data (from SMPTE 2016-3), saving data for future use
                                        #if defined(MEDIAINFO_AFDBARDATA_YES)
                                        {
                                            buffered_data* AfdBarData=new buffered_data;
                                            AfdBarData->Data=new int8u[(size_t)DataCount];
                                            std::memcpy(AfdBarData->Data, Payload, (size_t)DataCount);
                                            AfdBarData->Size=(size_t)DataCount;
                                            AfdBarData_Data.push_back(AfdBarData);
                                        }
                                        #endif //MEDIAINFO_AFDBARDATA_YES
                                        break;
                            case 0x06 : //SMPTE ST 2016
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Pan-Scan Information";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 2016";
                                        }
                                        break;
                            case 0x07 : //SMPTE ST 2010
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="ANSI/SCTE 104 Messages";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 2010";
                                        }
                                        break;
                            case 0x08 : //SMPTE ST 2031
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="DVB/SCTE VBI Data";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 2031";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x43 :
                        switch (SecondaryDataID)
                        {
                            case 0x02 : //OP-47 SDP, also RDD 8
                                        #if defined(MEDIAINFO_SDP_YES)
                                        if (Sdp_Parser==NULL)
                                        {
                                            Sdp_Parser=new File_Sdp;
                                            Open_Buffer_Init(Sdp_Parser);
                                        }
                                        if (!Sdp_Parser->Status[IsFinished])
                                        {
                                            if (Sdp_Parser->PTS_DTS_Needed)
                                                Sdp_Parser->FrameInfo=FrameInfo;
                                            Open_Buffer_Continue(Sdp_Parser, Payload, (size_t)DataCount);
                                        }
                                        #endif //defined(MEDIAINFO_SDP_YES)
                                        break;
                            case 0x03 : //OP-47 Multipacket, also RDD 8
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / OP-47 / Multipacket";
                                        }
                                        break;
                            case 0x05 : //RDD 18
                                        #if defined(MEDIAINFO_MXF_YES)
                                        if (Rdd18_Parser==NULL)
                                        {
                                            Rdd18_Parser=new File_Mxf;
                                            Open_Buffer_Init(Rdd18_Parser);
                                        }
                                        if (!Rdd18_Parser->Status[IsFinished])
                                            Open_Buffer_Continue(Rdd18_Parser, Payload+1, (size_t)DataCount-1);
                                        #endif //defined(MEDIAINFO_MXF_YES)
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x44 :
                        switch (SecondaryDataID)
                        {
                            case 0x44 : //SMPTE RP 223
                                        if (TestAndPrepare())
                                        {
                                            switch (DataCount)
                                            {
                                                case 0x19: Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="ISAN"; break;
                                                case 0x20:
                                                case 0x40: Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="UMID"; break;
                                            }

                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE RP 223";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x45 : // (from SMPTE 2020-1)
                        switch (SecondaryDataID)
                        {
                            case 0x01 : //No association
                            case 0x02 : //Channel pair 1/2
                            case 0x03 : //Channel pair 3/4
                            case 0x04 : //Channel pair 5/6
                            case 0x05 : //Channel pair 7/8
                            case 0x06 : //Channel pair 9/10
                            case 0x07 : //Channel pair 11/12
                            case 0x08 : //Channel pair 13/14
                            case 0x09 : //Channel pair 15/16
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Audio Metadata";
                                            if (SecondaryDataID>1)
                                                Unknown[DataID][SecondaryDataID][string()].Infos["Format_Settings"]=__T("Channel pair ")+Ztring::ToZtring((SecondaryDataID-1)*2-1)+__T('/')+Ztring::ToZtring((SecondaryDataID-1)*2);
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 2020";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x46 :
                        switch (SecondaryDataID)
                        {
                            case 0x01 : // (from SMPTE ST 2051)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Two-Frame Marker";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 2051";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x50 :
                        switch (SecondaryDataID)
                        {
                            case 0x01: //RDD 8
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="WSS"; //TODO: inject it in the video stream when a sample is available
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / RDD 8";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x5F : // (from ARIB STD-B37)
                        if ((SecondaryDataID&0xF0)==0xD0) //Digital Closed Caption
                        {
                            #if defined(MEDIAINFO_ARIBSTDB24B37_YES)
                            if (AribStdB34B37_Parser==NULL)
                            {
                                AribStdB34B37_Parser=new File_AribStdB24B37;
                                ((File_AribStdB24B37*)AribStdB34B37_Parser)->IsAncillaryData=true;
                                ((File_AribStdB24B37*)AribStdB34B37_Parser)->ParseCcis=true;
                                Open_Buffer_Init(AribStdB34B37_Parser);
                            }
                            if (!AribStdB34B37_Parser->Status[IsFinished])
                            {
                                if (AribStdB34B37_Parser->PTS_DTS_Needed)
                                    AribStdB34B37_Parser->FrameInfo=FrameInfo;
                                Open_Buffer_Continue(AribStdB34B37_Parser, Payload, (size_t)DataCount);
                            }
                            #endif //defined(MEDIAINFO_ARIBSTDB24B37_YES)
                        }
                        else
                            if (TestAndPrepare())
                            {
                                Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                            }
                        break;
            case 0x60 :
                        switch (SecondaryDataID)
                        {
                            case 0x60 : // (from SMPTE RP 188 / SMPTE ST 12-2)
                                        // Time code ATC
                                        #if defined(MEDIAINFO_TIMECODE_YES)
                                        {
                                        File_Gxf_TimeCode Parser;
                                        Parser.IsAtc=true;
                                        Open_Buffer_Init(&Parser);
                                        Open_Buffer_Continue(&Parser, Payload, (size_t)DataCount);

                                        string Unique=Ztring().From_Number(LineNumber).To_UTF8()+(LineNumber_IsSecondField?"IsSecondField":"")+Parser.Settings;
                                        if (TestAndPrepare(&Unique))
                                        {
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Type"]="Time code";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Format"]="SMPTE ATC";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["TimeCode_FirstFrame"].From_UTF8(Parser.TimeCode_FirstFrame.c_str());
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["TimeCode_Settings"].From_UTF8(Parser.Settings.c_str());
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["MuxingMode"]="Ancillary data / SMPTE RP 188";
                                            if (LineNumber!=(int32u)-1)
                                                Unknown[DataID][SecondaryDataID][Unique].Infos["ID"]=__T("Line")+Ztring::ToZtring(LineNumber);
                                            if (LineNumber_IsSecondField)
                                                Unknown[DataID][SecondaryDataID][Unique].Infos["IsSecondField"]="Yes";
                                        }
                                        }
                                        #endif //defined(MEDIAINFO_TIMECODE_YES)
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x61 : //Defined data services (from SMPTE 334-1)
                        switch (SecondaryDataID)
                        {
                            case 0x01 : //CDP (from SMPTE 334-1)
                                        #if defined(MEDIAINFO_CDP_YES)
                                        {
                                            if (Cdp_Parser==NULL)
                                            {
                                                Cdp_Parser=new File_Cdp;
                                                Open_Buffer_Init(Cdp_Parser);
                                            }
                                            Demux(Payload, (size_t)DataCount, ContentType_MainStream);
                                            if (InDecodingOrder || (!HasBFrames && AspectRatio && FrameRate))
                                            {
                                                if (!Cdp_Parser->Status[IsFinished])
                                                {
                                                    if (Cdp_Parser->PTS_DTS_Needed)
                                                        Cdp_Parser->FrameInfo.DTS=FrameInfo.DTS;
                                                    ((File_Cdp*)Cdp_Parser)->AspectRatio=AspectRatio;
                                                    Open_Buffer_Continue(Cdp_Parser, Payload, (size_t)DataCount);
                                                }
                                            }
                                            else
                                            {
                                                //Saving data for future use
                                                buffered_data* Cdp=new buffered_data;
                                                Cdp->Data=new int8u[(size_t)DataCount];
                                                std::memcpy(Cdp->Data, Payload, (size_t)DataCount);
                                                Cdp->Size=(size_t)DataCount;
                                                Cdp_Data.push_back(Cdp);
                                            }
                                        }
                                        #endif //MEDIAINFO_CDP_YES
                                        break;
                            case 0x02 : //CEA-608 (from SMPTE 334-1)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].StreamKind=Stream_Text;
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="CEA-608";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE 334";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x62 : //Variable-format data services (from SMPTE 334-1)
                        switch (SecondaryDataID)
                        {
                            case 0x01 : //Program description (from SMPTE 334-1),
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Program description";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 334";
                                        }
                                        break;
                            case 0x02 : //Data broadcast (from SMPTE 334-1)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="Data broadcast";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 334";
                                        }
                                        break;
                            case 0x03 : //VBI data (from SMPTE 334-1)
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]="VBI data";
                                            Unknown[DataID][SecondaryDataID][string()].Infos["MuxingMode"]="Ancillary data / SMPTE ST 334";
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x64 :
                        switch (SecondaryDataID)
                        {
                            case 0x64 : // (from SMPTE RP 196)
                                        // LTC in HANC space
                                        {
                                        string Unique=Ztring().From_Number(LineNumber).To_UTF8();
                                        if (TestAndPrepare(&Unique))
                                        {
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Type"]="Time code";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Format"]="LTC";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["MuxingMode"]="Ancillary data / SMPTE RP 196";
                                            if (LineNumber!=(int32u)-1)
                                                Unknown[DataID][SecondaryDataID][Unique].Infos["ID"]=__T("Line")+Ztring::ToZtring(LineNumber);
                                        }
                                        }
                                        break;
                            case 0x7F : // (from SMPTE RP 196)
                                        // VITC in HANC space
                                        {
                                        string Unique=Ztring().From_Number(LineNumber).To_UTF8();
                                        if (TestAndPrepare(&Unique))
                                        {
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Type"]="Time code";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["Format"]="VITC";
                                            Unknown[DataID][SecondaryDataID][Unique].Infos["MuxingMode"]="Ancillary data / SMPTE RP 196";
                                            if (LineNumber!=(int32u)-1)
                                                Unknown[DataID][SecondaryDataID][Unique].Infos["ID"]=__T("Line")+Ztring::ToZtring(LineNumber);
                                        }
                                        }
                                        break;
                            default   :
                                        if (TestAndPrepare())
                                        {
                                            Unknown[DataID][SecondaryDataID][string()].Infos["Format"]=Ztring().From_CC1(DataID)+__T('-')+Ztring().From_CC1(SecondaryDataID);
                                        }
                        }
                        break;
            case 0x00 : // Undefined format
            case 0x80 : // Marked for deletion
            case 0x84 : // End marker
            case 0x88 : // Start marker
                        break;
            default   :
                        if (TestAndPrepare())
                        {
                            Unknown[DataID][(DataID<0x80?SecondaryDataID:0)][string()].Infos["Format"]=Ztring().From_CC1(DataID)+((DataID<0x80)?(__T('-')+Ztring().From_CC1(SecondaryDataID)):Ztring());
                        }
        }
    FILLING_END();

    delete[] Payload; //Payload=NULL
}

//***************************************************************************
// Presence
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ancillary::TestAndPrepare(const string* Unique)
{
    if (DataID>=Unknown.size())
        Unknown.resize(DataID+1);
    int8u RealSecondaryDataID=(DataID<0x80?SecondaryDataID:0);
    if (RealSecondaryDataID>=Unknown[DataID].size())
        Unknown[DataID].resize(RealSecondaryDataID+1);
    if (Unique)
    {
        perid::iterator Item = Unknown[DataID][RealSecondaryDataID].find(*Unique);
        if (Item!=Unknown[DataID][RealSecondaryDataID].end())
            return false;
    }
    else
    {
        if (!Unknown[DataID][RealSecondaryDataID].empty())
            return false;
    }

    return true;
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_ANCILLARY_YES

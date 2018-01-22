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
#if defined(MEDIAINFO_PBCORE_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Export/Export_PBCore.h"
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
Ztring PBCore_MediaType(MediaInfo_Internal &MI)
{
         if (MI.Count_Get(Stream_Video))
        return __T("Video");
    else if (MI.Count_Get(Stream_Audio))
        return __T("Sound");
    else if (MI.Count_Get(Stream_Image))
        return __T("Static Image");
    else if (MI.Count_Get(Stream_Text))
        return __T("Text");
    else
        return Ztring();
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Export_PBCore::Export_PBCore ()
{
}

//---------------------------------------------------------------------------
Export_PBCore::~Export_PBCore ()
{
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
void PBCore_Transform(Node *Parent, MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos)
{
    //Menu: only if TimeCode
    if (StreamKind==Stream_Menu && MI.Get(Stream_Menu, StreamPos, Menu_Format)!=__T("TimeCode"))
        return;

    //essenceTrackType
    Ztring essenceTrackType;
    switch (StreamKind)
    {
        case Stream_Video:
                            essenceTrackType=__T("Video");
                            break;
        case Stream_Audio:
                            essenceTrackType=__T("Audio");
                            break;
        case Stream_Text:
                            {
                            Ztring Format=MI.Get(Stream_Text, StreamPos, Text_Format);
                            if (Format==__T("EIA-608") || Format==__T("EIA-708"))
                                essenceTrackType=__T("CC");
                            else
                                essenceTrackType=__T("Text");
                            }
                            break;
        case Stream_Menu:
                            if (MI.Get(Stream_Menu, StreamPos, Menu_Format)==__T("TimeCode"))
                            {
                                essenceTrackType=__T("TimeCode");
                                break;
                            }
                            else
                                return; //Not supported
        default:            return; //Not supported
    }

    Node* Node_EssenceTrack=Parent->Add_Child("pbcoreEssenceTrack");

    Node_EssenceTrack->Add_Child("essenceTrackType", essenceTrackType);

    //essenceTrackIdentifier
    if (!MI.Get(StreamKind, StreamPos, __T("ID")).empty())
    {
        Node_EssenceTrack->Add_Child("essenceTrackIdentifier", MI.Get(StreamKind, StreamPos, __T("ID")));
        Node_EssenceTrack->Add_Child("essenceTrackIdentifierSource", "ID (Mediainfo)");
    }
    else if (!MI.Get(StreamKind, StreamPos, __T("UniqueID")).empty())
    {
        Node_EssenceTrack->Add_Child("essenceTrackIdentifier", MI.Get(StreamKind, StreamPos, __T("UniqueID")));
        Node_EssenceTrack->Add_Child("essenceTrackIdentifierSource", "UniqueID (Mediainfo)");
    }
    else if (!MI.Get(StreamKind, StreamPos, __T("StreamKindID")).empty())
    {
        Node_EssenceTrack->Add_Child("essenceTrackIdentifier", MI.Get(StreamKind, StreamPos, __T("StreamKindID")));
        Node_EssenceTrack->Add_Child("essenceTrackIdentifierSource", "StreamKindID (Mediainfo)");
    }

    //essenceTrackStandard
    if (StreamKind==Stream_Video)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Standard, "essenceTrackStandard");

    //essenceTrackEncoding
    if (!MI.Get(StreamKind, StreamPos, __T("Format")).empty())
    {
        Ztring Format=MI.Get(StreamKind, StreamPos, __T("Format"));
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Profile")).empty())
            Format+=__T(' ')+MI.Get(StreamKind, StreamPos, __T("Format_Profile"));
        if (!MI.Get(StreamKind, StreamPos, __T("CodecID")).empty())
            Format+=__T(" (")+MI.Get(StreamKind, StreamPos, __T("CodecID"))+__T(')');
        Node_EssenceTrack->Add_Child("essenceTrackEncoding", Format);
    }

    //essenceTrackDataRate
    if (!MI.Get(StreamKind, StreamPos, __T("BitRate")).empty())
    {
        Ztring BitRate=MI.Get(StreamKind, StreamPos, __T("BitRate"));
        if (!MI.Get(StreamKind, StreamPos, __T("BitRate_Mode")).empty())
            BitRate+=__T(' ')+MI.Get(StreamKind, StreamPos, __T("BitRate_Mode"));
        Node_EssenceTrack->Add_Child("essenceTrackDataRate", BitRate);
    }

    //essenceTrackFrameRate
    if (StreamKind==Stream_Video && !MI.Get(Stream_Video, StreamPos, Video_FrameRate).empty())
    {
        Ztring FrameRate=MI.Get(Stream_Video, StreamPos, Video_FrameRate);
        if (!MI.Get(Stream_Video, StreamPos, Video_FrameRate_Mode).empty())
            FrameRate+=__T(' ')+MI.Get(Stream_Video, StreamPos, Video_FrameRate_Mode);
        Node_EssenceTrack->Add_Child("essenceTrackFrameRate", FrameRate);
    }

    //essenceTrackSamplingRate
    if (StreamKind==Stream_Audio)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_SamplingRate, "essenceTrackSamplingRate");

    //essenceTrackBitDepth
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "BitDepth", "essenceTrackBitDepth", "version", "PBCoreXSD_Ver_1.2_D1");

    //essenceTrackFrameSize
    if (StreamKind==Stream_Video && !MI.Get(Stream_Video, StreamPos, Video_Width).empty())
        Node_EssenceTrack->Add_Child("essenceTrackFrameSize", MI.Get(Stream_Video, StreamPos, Video_Width)+__T('x')+MI.Get(Stream_Video, StreamPos, Video_Height));

    //essenceTrackAspectRatio
    if (StreamKind==Stream_Video)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_DisplayAspectRatio, "essenceTrackAspectRatio");

    //essenceTrackDuration
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "Duration", "essenceTrackDuration");

    //essenceTrackLanguage
    if (!MI.Get(StreamKind, StreamPos, __T("Language")).empty())
        Node_EssenceTrack->Add_Child("essenceTrackLanguage", MediaInfoLib::Config.Iso639_2_Get(MI.Get(StreamKind, StreamPos, __T("Language"))));

    //essenceTrackAnnotation - all fields (except *_String*) separated by |
    Ztring Temp;
    for (size_t Pos=0; Pos<MI.Count_Get(StreamKind, StreamPos); Pos++)
        if (MI.Get(StreamKind, StreamPos, Pos, Info_Name).find(__T("String"))==std::string::npos && !MI.Get(StreamKind, StreamPos, Pos).empty())
            Temp+=MI.Get(StreamKind, StreamPos, Pos, Info_Name)+__T(": ")+MI.Get(StreamKind, StreamPos, Pos)+__T('|');
    if (!Temp.empty())
    {
        Temp.resize(Temp.size()-1);
        Node_EssenceTrack->Add_Child("essenceTrackAnnotation", Temp);
    }
}

//---------------------------------------------------------------------------
Ztring Export_PBCore::Transform(MediaInfo_Internal &MI)
{


    Node Node_Main("PBCoreDescriptionDocument");
    Node_Main.Add_Attribute("xsi:schemaLocation", "http://www.pbcore.org/PBCore/PBCoreNamespace.html http://www.pbcore.org/PBCore/PBCoreXSD_Ver_1-2-1.xsd");
    Node_Main.Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    Node_Main.Add_Attribute("xmlns", "http://www.pbcore.org/PBCore/PBCoreNamespace.html");
    Node* Node_Identifier = Node_Main.Add_Child("pbcoreIdentifier");
    Node_Identifier->Add_Child("identifier", "***");
    Node_Identifier->Add_Child("identifierSource", "***");

    Node_Main.Add_Child("pbcoreTitle")->Add_Child("title", "***");

    Node* Node_Description = Node_Main.Add_Child("pbcoreDescription");
    Node_Description->Add_Child("description", "***");
    Node_Description->Add_Child("descriptionType", "***");
    
    Node* Node_Instantiation = Node_Main.Add_Child("pbcoreInstantiation");
    
    //pbcoreFormatID
    Node* Node_FormatID = Node_Instantiation->Add_Child("pbcoreFormatID");
    //formatIdentifier
    Node_FormatID->Add_Child("formatIdentifier", MI.Get(Stream_General, 0, General_FileName));
    //formatIdentifierSource
    Node_FormatID->Add_Child("formatIdentifierSource", "File Name", "version", "PBCoreXSD_Ver_1.2_D1");

    //formatDigital
    //TODO: how to implement formats without Media Type?
    Ztring Format;
    if (!MI.Get(Stream_General, 0, General_InternetMediaType).empty())
        Format=Ztring(MI.Get(Stream_General, 0, General_InternetMediaType));
    else if (MI.Count_Get(Stream_Video))
        Format=__T("video/x-")+Ztring(MI.Get(Stream_General, 0, __T("Format"))).MakeLowerCase();
    else if (MI.Count_Get(Stream_Image))
        Format=__T("image/x-")+Ztring(MI.Get(Stream_General, 0, __T("Format"))).MakeLowerCase();
    else if (MI.Count_Get(Stream_Audio))
        Format=__T("audio/x-")+Ztring(MI.Get(Stream_General, 0, __T("Format"))).MakeLowerCase();
    else
        Format=__T("application/x-")+Ztring(MI.Get(Stream_General, 0, __T("Format"))).MakeLowerCase();
    Node_Instantiation->Add_Child("formatDigital", Format);

    //formatLocation
    Node_Instantiation->Add_Child("formatLocation", MI.Get(Stream_General, 0, General_CompleteName));

    //dateCreated
    if (!MI.Get(Stream_General, 0, General_Encoded_Date).empty())
    {
        Ztring dateCreated=MI.Get(Stream_General, 0, General_Recorded_Date);
        dateCreated.FindAndReplace(__T("UTC"), __T("-"));
        dateCreated.FindAndReplace(__T(" "), __T("T"));
        dateCreated+=__T('Z');
        Node_Instantiation->Add_Child("dateCreated", dateCreated);
    }

    //dateIssued
    if (!MI.Get(Stream_General, 0, General_Recorded_Date).empty())
    {
        Ztring dateIssued=MI.Get(Stream_General, 0, General_Recorded_Date);
        dateIssued.FindAndReplace(__T("UTC"), __T("-"));
        dateIssued.FindAndReplace(__T(" "), __T("T"));
        dateIssued+=__T('Z');
        Node_Instantiation->Add_Child("dateIssued", dateIssued);
    }

    //formatMediaType
    Node_Instantiation->Add_Child("formatMediaType", PBCore_MediaType(MI).empty()?Ztring(__T("application/octet-stream")):PBCore_MediaType(MI), "version", "PBCoreXSD_Ver_1.2_D1");

    //formatGenerations
    Node_Instantiation->Add_Child("formatGenerations", "","version", "PBCoreXSD_Ver_1.2_D1");

    //formatFileSize
    Node_Instantiation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_FileSize, "formatFileSize");

    //formatTimeStart
    if (!MI.Get(Stream_Video, 0, Video_Delay_Original_String3).empty())
        Node_Instantiation->Add_Child("formatTimeStart", MI.Get(Stream_Video, 0, Video_Delay_Original_String3));
    else if (!MI.Get(Stream_Video, 0, Video_Delay_String3).empty())
        Node_Instantiation->Add_Child("formatTimeStart", MI.Get(Stream_Video, 0, Video_Delay_String3));

    //formatDuration
    Node_Instantiation->Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Duration_String3, "formatDuration");

    //formatDataRate
    if (!MI.Get(Stream_General, 0, General_OverallBitRate).empty())
    {
        Ztring formatDataRate=MI.Get(Stream_General, 0, General_OverallBitRate);
        if (!MI.Get(Stream_General, 0, General_OverallBitRate_Mode).empty())
            formatDataRate+=__T(' ')+MI.Get(Stream_General, 0, General_OverallBitRate_Mode);
        Node_Instantiation->Add_Child("formatDataRate", formatDataRate);
    }

    //formatTracks
    Node_Instantiation->Add_Child("formatTracks", Ztring::ToZtring(MI.Count_Get(Stream_Video)+MI.Count_Get(Stream_Audio)+MI.Count_Get(Stream_Image)+MI.Count_Get(Stream_Text)));

    Ztring ToReturn;

    //Streams
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
            PBCore_Transform(Node_Instantiation, MI, (stream_t)StreamKind, StreamPos);

    ToReturn+=Ztring().From_UTF8(To_XML(Node_Main, 0).c_str());

    size_t Pos=ToReturn.find(__T("<PBCoreDescriptionDocument"));
    if(Pos!=Ztring::npos)
        ToReturn.insert(Pos, __T("<!-- Warning: MediaInfo outputs only pbcoreInstantiation, other mandatory PBCore data is junk -->\n"));

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

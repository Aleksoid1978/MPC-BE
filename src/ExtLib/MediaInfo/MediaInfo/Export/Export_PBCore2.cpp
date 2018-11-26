/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Contributor: Dave Rice, dave@dericed.com
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#include "MediaInfo/Export/Export_PBCore2.h"
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
Ztring PBCore2_MediaType(MediaInfo_Internal &MI)
{
    if (MI.Count_Get(Stream_Video))
        return __T("Moving Image");
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
Export_PBCore2::Export_PBCore2 ()
{
}

//---------------------------------------------------------------------------
Export_PBCore2::~Export_PBCore2 ()
{
}

//***************************************************************************
// Input
//***************************************************************************

//---------------------------------------------------------------------------
void PBCore2_Transform(Node *Parent, MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos)
{
Ztring ToReturn;
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
        case Stream_Image:
            essenceTrackType=__T("Image");
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

    Node* Node_EssenceTrack=Parent->Add_Child("instantiationEssenceTrack");

    Node_EssenceTrack->Add_Child("essenceTrackType", essenceTrackType);

    //essenceTrackIdentifier
    if (!MI.Get(StreamKind, StreamPos, __T("ID")).empty())
    {
        Node* Child=Node_EssenceTrack->Add_Child("essenceTrackIdentifier", MI.Get(StreamKind, StreamPos, __T("ID")));
        Child->Add_Attribute("source", std::string("ID"));
        Ztring id_annotation;
        if (!MI.Get(StreamKind, StreamPos, __T("Default")).empty())
            id_annotation+=__T(" default:")+MI.Get(StreamKind, StreamPos, __T("Default"));
        if (!MI.Get(StreamKind, StreamPos, __T("Forced")).empty())
            id_annotation+=__T(" forced:")+MI.Get(StreamKind, StreamPos, __T("Forced"));
        id_annotation=id_annotation.erase(0,1);
        if (!id_annotation.empty())
            Child->Add_Attribute("annotation", id_annotation);
    }
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "UniqueID", "essenceTrackIdentifier", "source", std::string("UniqueID"));
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "MenuID", "essenceTrackIdentifier", "source", std::string("MenuID"));
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "StreamKindID", "essenceTrackIdentifier", "source", std::string("StreamKindID (MediaInfo)"));
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "StreamOrder", "essenceTrackIdentifier", "source", std::string("StreamOrder (MediaInfo)"));

    //essenceTrackStandard
    if (StreamKind==Stream_Video)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Standard, "essenceTrackStandard");

    //essenceTrackEncoding
    if (!MI.Get(StreamKind, StreamPos, __T("Format")).empty())
    {
        Node* Child=Node_EssenceTrack->Add_Child("essenceTrackEncoding", MI.Get(StreamKind, StreamPos, __T("Format")));
        if (!MI.Get(StreamKind, StreamPos, __T("CodecID")).empty())
        {
            Child->Add_Attribute("source", "codecid");
            Child->Add_Attribute("ref", MI.Get(StreamKind, StreamPos, __T("CodecID")));
        }
        Child->Add_Attribute_IfNotEmpty(MI, StreamKind, StreamPos, "Format_Version", "version");

        Ztring encoding_annotation;
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Profile")).empty())
            encoding_annotation+=__T(" profile:")+MI.Get(StreamKind, StreamPos, __T("Format_Profile"));
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Settings_Endianness")).empty())
            encoding_annotation+=__T(" endianness:")+MI.Get(StreamKind, StreamPos, __T("Format_Settings_Endianness"));
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Settings_Sign")).empty())
            encoding_annotation+=__T(" signedness:")+MI.Get(StreamKind, StreamPos, __T("Format_Settings_Sign"));
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Settings_Packing")).empty())
            encoding_annotation+=__T(" packing:")+MI.Get(StreamKind, StreamPos, __T("Format_Settings_Packing"));
        if (!MI.Get(StreamKind, StreamPos, __T("Format_Compression")).empty())
            encoding_annotation+=__T(" compression:")+MI.Get(StreamKind, StreamPos, __T("Format_Compression"));
        if (!MI.Get(StreamKind, StreamPos, __T("Compression_Mode")).empty())
            encoding_annotation+=__T(" compression_mode:")+MI.Get(StreamKind, StreamPos, __T("Compression_Mode"));
        encoding_annotation=encoding_annotation.erase(0,1);
        if (!encoding_annotation.empty())
            Child->Add_Attribute("annotation", encoding_annotation);
    }

    //essenceTrackDataRate
    if (!MI.Get(StreamKind, StreamPos, __T("BitRate")).empty())
    {
        Node* Child=Node_EssenceTrack->Add_Child("essenceTrackDataRate", MI.Get(StreamKind, StreamPos, __T("BitRate")));
        Child->Add_Attribute("unitsOfMeasure", "bit/second");
        Child->Add_Attribute_IfNotEmpty(MI, StreamKind, StreamPos, "BitRate_Mode", "annotation");
    }

    //essenceTrackFrameRate
    if (StreamKind==Stream_Video && !MI.Get(Stream_Video, StreamPos, Video_FrameRate).empty())
    {
        Node* Child=Node_EssenceTrack->Add_Child("essenceTrackFrameRate", MI.Get(Stream_Video, StreamPos, Video_FrameRate));

        Ztring frame_rate_annotation;
        if (!MI.Get(StreamKind, StreamPos, __T("Video_FrameRate_Mode")).empty())
            frame_rate_annotation+=__T(" mode:")+MI.Get(StreamKind, StreamPos, __T("Video_FrameRate_Mode"));
        if (!MI.Get(StreamKind, StreamPos, __T("FrameRate_Num")).empty())
            frame_rate_annotation+=__T(" rational_frame_rate:")+MI.Get(StreamKind, StreamPos, __T("FrameRate_Num"))+__T("/")+MI.Get(StreamKind, StreamPos, __T("FrameRate_Den"));
        if (!MI.Get(StreamKind, StreamPos, __T("ScanOrder")).empty())
            frame_rate_annotation+=__T(" interlacement:")+MI.Get(StreamKind, StreamPos, __T("ScanOrder"));
        else if (!MI.Get(StreamKind, StreamPos, __T("ScanType")).empty())
            frame_rate_annotation+=__T(" interlacement:")+MI.Get(StreamKind, StreamPos, __T("ScanType"));
        frame_rate_annotation=frame_rate_annotation.erase(0,1);
        if (!frame_rate_annotation.empty())
            Child->Add_Attribute("annotation", frame_rate_annotation);
    }

    //essenceTrackSamplingRate
    if (StreamKind==Stream_Audio)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_SamplingRate, "essenceTrackSamplingRate", "unitsOfMeasure", std::string("Hz"));

    //essenceTrackBitDepth
    Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "BitDepth", "essenceTrackBitDepth");

    //essenceTrackFrameSize
    if (!MI.Get(StreamKind, StreamPos, __T("Width")).empty())
    {
        Ztring framesize;
        framesize=MI.Get(StreamKind, StreamPos, __T("Width"));
        framesize+=__T("x");
        framesize+=MI.Get(StreamKind, StreamPos, __T("Height"));
        Node_EssenceTrack->Add_Child("essenceTrackFrameSize", framesize);
    }

    //essenceTrackAspectRatio
    if (StreamKind==Stream_Video)
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_DisplayAspectRatio, "essenceTrackAspectRatio");

    //essenceTrackTimeStart
    if (StreamKind==Stream_Video)
    {
        if (!MI.Get(Stream_Video, StreamPos, Video_Delay_Original_String4).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Delay_Original_String4, "essenceTrackTimeStart", "source", std::string("from encoding"));
        else if (!MI.Get(Stream_Video, StreamPos, Video_Delay_Original_String3).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Delay_Original_String3, "essenceTrackTimeStart", "source", std::string("from encoding"));
        else if (!MI.Get(Stream_Video, StreamPos, Video_Delay_String4).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Delay_String4, "essenceTrackTimeStart", "source", MI.Get(Stream_Video, StreamPos, Video_Delay_Source).To_UTF8());
        else if (!MI.Get(Stream_Video, StreamPos, Video_Delay_String3).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Video, StreamPos, Video_Delay_String3, "essenceTrackTimeStart", "source", MI.Get(Stream_Video, StreamPos, Video_Delay_Source).To_UTF8());
    }
    else if (StreamKind==Stream_Audio)
    {
        if (!MI.Get(Stream_Audio, StreamPos, Audio_Delay_Original_String4).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Delay_Original_String4, "essenceTrackTimeStart", "source", std::string("from encoding"));
        else if (!MI.Get(Stream_Audio, StreamPos, Audio_Delay_Original_String3).empty())
            Node_EssenceTrack->Add_Child_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Delay_Original_String3, "essenceTrackTimeStart", "source", std::string("from encoding"));
        else if (!MI.Get(Stream_Audio, StreamPos, Audio_Delay_String4).empty())
        {
            Node* Child=Node_EssenceTrack->Add_Child("essenceTrackTimeStart", MI.Get(Stream_Audio, StreamPos, Audio_Delay_String4));
            Child->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Delay_Source, "source");
        }
        else if (!MI.Get(Stream_Audio, StreamPos, Audio_Delay_String3).empty())
        {
            Node* Child=Node_EssenceTrack->Add_Child("essenceTrackTimeStart", MI.Get(Stream_Audio, StreamPos, Audio_Delay_String3));
            Child->Add_Attribute_IfNotEmpty(MI, Stream_Audio, StreamPos, Audio_Delay_Source, "source");
        }
    }

    //essenceTrackDuration
    if (!MI.Get(StreamKind, StreamPos, __T("Duration_String4")).empty())
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "Duration_String4", "essenceTrackDuration");
    else
        Node_EssenceTrack->Add_Child_IfNotEmpty(MI, StreamKind, StreamPos, "Duration_String3", "essenceTrackDuration");

    //essenceTrackLanguage
    if (!MI.Get(StreamKind, StreamPos, __T("Language")).empty())
        Node_EssenceTrack->Add_Child("essenceTrackLanguage", MediaInfoLib::Config.Iso639_2_Get(MI.Get(StreamKind, StreamPos, __T("Language"))));

    //essenceTrackAnnotation - all fields (except *_String* and a blacklist)
    for (size_t Pos=0; Pos<MI.Count_Get(StreamKind, StreamPos); Pos++)
        if (
            !MI.Get(StreamKind, StreamPos, Pos).empty() &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("BitDepth") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("BitRate") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("BitRate_Mode") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Bits-(Pixel*Frame)") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Channel(s)") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("ChannelLayout") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("ChannelPositions") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec/CC") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec/Family") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec/Info") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec/Url") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("CodecID") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("CodecID/Info") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("CodecID/Url") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Profile") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings_CABAC") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings_Endianness") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings_Floor") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings_RefFrames") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Codec_Settings_Sign") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Colorimetry") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Compression_Mode") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Count") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Default") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Delay") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Delay_DropFrame") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Delay_Settings") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Delay_Source") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Density_Unit") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Density_X") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Density_Y") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("DisplayAspectRatio") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Duration") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Encoded_Date") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Encoded_Library") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Forced") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Compression") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Settings") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Settings_Packing") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format/Info") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format/Url") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Commercial") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Profile") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Settings_Endianness") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Settings_Sign") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Format_Version") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("FrameRate") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("FrameRate_Den") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("FrameRate_Mode") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("FrameRate_Num") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Height") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("ID") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("InternetMediaType") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Language") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("MenuID") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("PixelAspectRatio") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Resolution") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Sampled_Height") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Sampled_Width") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("SamplingCount") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("SamplingRate") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("ScanType") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("ScanOrder") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Standard") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamCount") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamKind") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamKindID") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamKindPos") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamOrder") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("StreamSize_Proportion") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Tagged_Date") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("UniqueID") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_Delay") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_DisplayAspectRatio") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_FrameRate") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_FrameRate_Mode") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_Height") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_Standard") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Video_Width") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name)!=__T("Width") &&
            MI.Get(StreamKind, StreamPos, Pos, Info_Name).find(__T("String"))==std::string::npos
            )
                Node_EssenceTrack->Add_Child("essenceTrackAnnotation", MI.Get(StreamKind, StreamPos, Pos),
                    "annotationType", MI.Get(StreamKind, StreamPos, Pos, Info_Name).To_UTF8());
}

//---------------------------------------------------------------------------
Ztring Export_PBCore2::Transform(MediaInfo_Internal &MI, version Version)
{
    Ztring ToReturn;

    Node Node_Main("pbcoreInstantiationDocument");
    if (Version==Version_2_0)
        Node_Main.Add_Attribute("xsi:schemaLocation", "http://www.pbcore.org/PBCore/PBCoreNamespace.html http://pbcore.org/xsd/pbcore-2.0.xsd");
    else
        Node_Main.Add_Attribute("xsi:schemaLocation", "http://www.pbcore.org/PBCore/PBCoreNamespace.html https://raw.githubusercontent.com/WGBH/PBCore_2.1/master/pbcore-2.1.xsd"); //TODO: better URL
    Node_Main.Add_Attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    Node_Main.Add_Attribute("xmlns", "http://www.pbcore.org/PBCore/PBCoreNamespace.html");

    //instantiationIdentifier
    Ztring instantiationIdentifier=MI.Get(Stream_General, 0, General_FileName);
    if (!MI.Get(Stream_General, 0, General_FileExtension).empty())
       instantiationIdentifier+=__T(".")+MI.Get(Stream_General, 0, General_FileExtension);

    Node_Main.Add_Child("instantiationIdentifier", instantiationIdentifier, "source", "File Name");

    // call UniqueID as SegmentUID for Matroska, else UniqueID
    if (MI.Get(Stream_General, 0, General_Format)==__T("Matroska"))
      Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_UniqueID, "instantiationIdentifier", "source", std::string("SegmentUID"));
    else if (MI.Get(Stream_General, 0, General_Format)==__T("P2 Clip"))
      Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_UniqueID, "instantiationIdentifier", "source", std::string("GlobalClipID"));
    else if (MI.Get(Stream_General, 0, General_Format)==__T("Windows Media"))
      Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_UniqueID, "instantiationIdentifier", "source", std::string("WM/UniqueFileIdentifier"));
    else
      Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_UniqueID, "instantiationIdentifier", "source", std::string("UniqueID"));

    // get final cut uuids as instantiation identifiers
    Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, "Media/UUID", "instantiationIdentifier", "source", std::string("com.apple.finalcutstudio.media.uuid"));

    // get broadcast wave umids as instantiation identifiers
    Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, "UMID", "instantiationIdentifier", "source", std::string("UMID"));

    //instantiationDates
    //dateIssued
    if (!MI.Get(Stream_General, 0, General_Recorded_Date).empty())
    {
        Ztring dateIssued=MI.Get(Stream_General, 0, General_Recorded_Date);
        dateIssued.FindAndReplace(__T("UTC"), __T(""));
        dateIssued.FindAndReplace(__T(" "), __T("T"));
        if (dateIssued.size()>=13)
            dateIssued+=__T('Z');
        Node_Main.Add_Child("instantiationDate", dateIssued, "dateType", "recorded");
    }

    //dateFileModified
    if (!MI.Get(Stream_General, 0, General_File_Modified_Date).empty())
    {
        Ztring dateModified=MI.Get(Stream_General, 0, General_File_Modified_Date);
        dateModified.FindAndReplace(__T("UTC "), __T(""));
        dateModified.FindAndReplace(__T(" "), __T("T"));
        if (dateModified.size()>=13)
            dateModified+=__T('Z');
        Node_Main.Add_Child("instantiationDate", dateModified, "dateType", "file modification");
    }

    //dateEncoder
    if (!MI.Get(Stream_General, 0, General_Encoded_Date).empty())
    {
        Ztring dateEncoded=MI.Get(Stream_General, 0, General_Encoded_Date);
        dateEncoded.FindAndReplace(__T("UTC "), __T(""));
        dateEncoded.FindAndReplace(__T(" "), __T("T"));
        if (dateEncoded.size()>=13)
            dateEncoded+=__T('Z');
        Node_Main.Add_Child("instantiationDate", dateEncoded, "dateType", "encoded");
    }

    //dateTagged
    if (!MI.Get(Stream_General, 0, General_Tagged_Date).empty())
    {
        Ztring dateTagged=MI.Get(Stream_General, 0, General_Tagged_Date);
        dateTagged.FindAndReplace(__T("UTC "), __T(""));
        dateTagged.FindAndReplace(__T(" "), __T("T"));
        if (dateTagged.size()>=13)
            dateTagged+=__T('Z');
        Node_Main.Add_Child("instantiationDate", dateTagged, "dateType", "tagged");
    }

    //instantiationDimensions
    if (!MI.Get(Stream_Image, 0, __T("Density_X")).empty())
    {
        Ztring dimensions;
        dimensions=MI.Get(Stream_Image, 0, __T("Density_X"));
        dimensions+=__T("x");
        dimensions+=MI.Get(Stream_Image, 0, __T("Density_Y"));
        Node* Child=Node_Main.Add_Child("instantiationDimensions", dimensions);
        Child->Add_Attribute_IfNotEmpty(MI, Stream_Image, 0, "Density_Unit", "unitsOfMeasure");
    }

    //instantiationDigital
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
    Node_Main.Add_Child("instantiationDigital", Format);

    //instantiationStandard
    Ztring formatStandard=MI.Get(Stream_General, 0, General_Format);
    if (!MI.Get(Stream_General, 0, General_Format_Commercial_IfAny).empty())
        formatStandard+=__T(" (")+MI.Get(Stream_General, 0, General_Format_Commercial_IfAny)+__T(")");
    Node* Child=Node_Main.Add_Child("instantiationStandard", formatStandard);
    Child->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_Format_Profile, "profile");
    Child->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_Format_Version, "annotation");

    //instantiationLocation
    if (!MI.Get(Stream_General, 0, __T("instantiationLocation")).empty())
        Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, "instantiationLocation", "instantiationLocation");
    else
        Node_Main.Add_Child("instantiationLocation", MI.Get(Stream_General, 0, General_CompleteName));

    //instantiationMediaType
    if (!PBCore2_MediaType(MI).empty())
        Node_Main.Add_Child("instantiationMediaType", PBCore2_MediaType(MI));

    //instantiationGenerations
    if (!MI.Get(Stream_General, 0, __T("instantiationGenerations")).empty())
        Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, "instantiationGenerations", "instantiationGenerations");

    //instantiationFileSize
    Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_FileSize, "instantiationFileSize", "unitsOfMeasure", std::string("byte"));

    //instantiationTimeStart
    if (!MI.Get(Stream_General, 0, General_Delay_String4).empty())
        Node_Main.Add_Child("instantiationTimeStart", MI.Get(Stream_General, 0, General_Delay_String4));
    else if (!MI.Get(Stream_General, 0, General_Delay_String3).empty())
        Node_Main.Add_Child("instantiationTimeStart", MI.Get(Stream_General, 0, General_Delay_String3));

    //instantiationDuration
    //TODO add annotation if duration/source_duration mismatch
    if (!MI.Get(Stream_General, 0, General_Duration_String4).empty())
        Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Duration_String4, "instantiationDuration");
    else
        Node_Main.Add_Child_IfNotEmpty(MI, Stream_General, 0, General_Duration_String3, "instantiationDuration");

    //instantiationDataRate
    if (!MI.Get(Stream_General, 0, General_OverallBitRate).empty())
    {
        Node* Child=Node_Main.Add_Child("instantiationDataRate", MI.Get(Stream_General, 0, General_OverallBitRate), "unitsOfMeasure", "bit/second");
        Child->Add_Attribute_IfNotEmpty(MI, Stream_General, 0, General_OverallBitRate_Mode, "annotation");
    }

    //instantiationTracks
    Node_Main.Add_Child("instantiationTracks", Ztring::ToZtring(MI.Count_Get(Stream_Video)+
        MI.Count_Get(Stream_Audio)+ MI.Count_Get(Stream_Image)+ MI.Count_Get(Stream_Text)));

    //instantiationChannelConfiguration
    Ztring channelConfiguration;
    for (size_t StreamKind=Stream_Audio; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
            if (MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_) == __T("1"))
            {
                channelConfiguration+=__T(", Track ") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_ID)) + __T(": ") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_)) + __T(" channel");
                if (!MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout).empty())
                    channelConfiguration+=__T(" (") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout)) + __T(")");
            }
            else if (!MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_).empty())
            {
                channelConfiguration+=__T(", Track ") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_ID)) + __T(": ") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_Channel_s_)) + __T(" channels");
                if (!MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout).empty())
                    channelConfiguration+=__T(" (") + Ztring(MI.Get(Stream_Audio, StreamPos, Audio_ChannelLayout)) + __T(")");
            }
    channelConfiguration=channelConfiguration.erase(0,2);
    if (!channelConfiguration.empty())
        Node_Main.Add_Child("instantiationChannelConfiguration", channelConfiguration);

    //Streams
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
            PBCore2_Transform(&Node_Main, MI, (stream_t)StreamKind, StreamPos);

    //instantiationRights
    if (!MI.Get(Stream_General, 0, General_Copyright).empty())
    {
        Node* instantiationRights=Node_Main.Add_Child("instantiationRights");
        instantiationRights->Add_Child("rightsSummary", MI.Get(Stream_General, 0, General_Copyright), "annotation", "embedded copyright statement");
    }

    //instantiationAnnotations
    for (size_t Pos=0; Pos<MI.Count_Get(Stream_General, 0); Pos++)
        if (
            !MI.Get(Stream_General, 0, Pos).empty() &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("AudioCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Audio_Codec_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Audio_Format_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Audio_Format_WithHint_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Audio_Language_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Codec") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Codec/Extensions") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Codec/Url") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("CodecID") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("CodecID/Url") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("CodecID_Compatible") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("CodecID_Version") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("CompleteName") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("com.apple.quicktime.player.movie.audio.mute") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Copyright") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Count") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("DataSize") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Duration") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Encoded_Date") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Encoded_Library_Name") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Encoded_Library_Version") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FileExtension") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FileName") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FileNameExtension") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FileSize") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("File_Modified_Date") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("File_Modified_Date_Local") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FolderName") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FooterSize") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format/Extensions") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format/Url") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format_Commercial") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format_Profile") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Format_Version") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("FrameRate") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("HeaderSize") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("ImageCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("instantiationGenerations") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("instantiationLocation") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("InternetMediaType") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("IsStreamable") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("MenuCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("OtherCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Other_Format_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Other_Format_WithHint_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Other_Language_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("OverallBitRate") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("OverallBitRate_Mode") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Recorded_Date") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("StreamCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("StreamKind") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("StreamKindID") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("StreamSize") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("StreamSize_Proportion") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Tagged_Date") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("TextCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Text_Codec_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Text_Format_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Text_Format_WithHint_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Text_Language_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("UMID") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("UniqueID") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("VideoCount") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Video_Codec_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name)!=__T("Video_Language_List") &&
            MI.Get(Stream_General, 0, Pos, Info_Name).find(__T("_Format_List"))==std::string::npos &&
            MI.Get(Stream_General, 0, Pos, Info_Name).find(__T("_Format_WithHint_List"))==std::string::npos &&
            MI.Get(Stream_General, 0, Pos, Info_Name).find(__T("String"))==std::string::npos
            )
                Node_Main.Add_Child("instantiationAnnotation", MI.Get(Stream_General, 0, Pos),
                    "annotationType", MI.Get(Stream_General, 0, Pos, Info_Name).To_UTF8());

    ToReturn+=Ztring().From_UTF8(To_XML(Node_Main, 0, true, true).c_str());

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

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
#if defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_DolbyVisionMetadata.h"
#include "tinyxml2.h"
#include <cstring>
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyVisionMetadata::File_DolbyVisionMetadata()
{
    //Configuration
    StreamSource=IsContainerExtra;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyVisionMetadata::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    string Version;
    float32 AspectRatio=0;
    XMLElement* DolbyVisionGlobalData = document.FirstChildElement();
    if (!DolbyVisionGlobalData)
    {
        Reject("DolbyVisionMetadata");
        return false;
    }

    XMLElement* MasterElements[2] = {};
    
    const char* Format = nullptr;
    if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:DolbyVisionGlobalDataGSP"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:Version"))
                Version = DolbyVisionGlobalData->GetText();
            if (!strcmp(DolbyVisionGlobalData->Name(), "gsp:Track"))
            {
                MasterElements[1]=DolbyVisionGlobalData;
                break;
            }
        }
        Format = "Dolby Vision Global Data GSP";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:DolbyVisionIntegratedData"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:Version"))
                Version = DolbyVisionGlobalData->GetText();
            if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:DolbyVisionGlobalData"))
            {
                for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
                {
                    if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:CanvasAspectRatio"))
                    {
                        if (const char* Text = DolbyVisionGlobalData->GetText())
                            if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                                AspectRatio = TextF;
                    }
                    if (!strcmp(DolbyVisionGlobalData->Name(), "dvmd-int:Track"))
                    {
                        MasterElements[1]=DolbyVisionGlobalData;
                        break;
                    }
                }
                break;
            }
        }
        Format = "Dolby Vision Integrated Data";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionIntegratedWrapper"))
    {
        for (DolbyVisionGlobalData = DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData; DolbyVisionGlobalData = DolbyVisionGlobalData->NextSiblingElement())
        {
            if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionGlobalData"))
            {
                MasterElements[0] = DolbyVisionGlobalData;
                Format = "Dolby Vision Global Data";
            }
            if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionFrameData"))
            {
                Format = "Dolby Vision Frame Data";
                MasterElements[1]=DolbyVisionGlobalData;
                break; // Frame Data has priority over Global Data
            }
        }
        if (!Format)
            Format = "Dolby Vision Integrated Wrapper";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionGlobalData"))
    {
        MasterElements[0] = DolbyVisionGlobalData;
        Format = "Dolby Vision Global Data";
    }
    else if (!strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionFrameData") || !strcmp(DolbyVisionGlobalData->Name(), "fw:DolbyVisionFrameData"))
    {
        MasterElements[1] = DolbyVisionGlobalData;
        Format = "Dolby Vision Frame Data";
    }
    else
        DolbyVisionGlobalData = NULL;

    if (!DolbyVisionGlobalData)
    {
        Reject("DolbyVisionMetadata");
        return false;
    }
    if (const char* Text = DolbyVisionGlobalData->Attribute("version"))
        Version=Text;

    Accept("DolbyVisionMetadata");
    Fill(Stream_General, 0, General_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_HDR_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Other);
    if (Format)
        Fill(Stream_Other, 0, Other_Format, Format);
    if (!Version.empty())
        Fill(Stream_Video, 0, Video_HDR_Format_Version, Version);
    if (AspectRatio)
        Fill(Stream_Video, 0, Video_DisplayAspectRatio, AspectRatio);

    for (auto MasterElement : MasterElements)
    {
    if (!MasterElement)
        continue;
    for (XMLElement* DolbyVisionGlobalData_Item=MasterElement->FirstChildElement(); DolbyVisionGlobalData_Item; DolbyVisionGlobalData_Item=DolbyVisionGlobalData_Item->NextSiblingElement())
    {
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "ColorEncoding"))
        {
            mastering_metadata_2086 Mastering;
            memset(&Mastering, 0xFF, sizeof(Mastering));
            for (XMLElement* ColorEncoding_Item=DolbyVisionGlobalData_Item->FirstChildElement(); ColorEncoding_Item; ColorEncoding_Item=ColorEncoding_Item->NextSiblingElement())
            {
                if (!strcmp(ColorEncoding_Item->Name(), "BitDepth"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        Fill(Stream_Video, 0, Video_BitDepth, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "CanvasAspectRatio"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                        if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                            Fill(Stream_Video, 0, Video_DisplayAspectRatio, TextF);
                }
                if (false) // !strcmp(ColorEncoding_Item->Name(), "ColorSpace"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "rgb"))
                            Text = "RGB";
                        if (!strcmp(Text, "yuv"))
                            Text = "YUV";
                        Fill(Stream_Video, 0, Video_ColorSpace, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "Encoding"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "pq"))
                            Text="PQ";
                        Fill(Stream_Video, 0, Video_transfer_characteristics, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "Primaries"))
                {
                    for (XMLElement* Primaries_Item=ColorEncoding_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                    {
                        int8u i=(int8u)-1;
                        if (!strcmp(Primaries_Item->Name(), "Green"))
                            i=0;
                        if (!strcmp(Primaries_Item->Name(), "Blue"))
                            i=2;
                        if (!strcmp(Primaries_Item->Name(), "Red"))
                            i=4;
                        if (i!=(int8u)-1)
                        {
                            if (const char* Text=Primaries_Item->GetText())
                            {
                                ZtringList List;
                                List.Separator_Set(0, __T(" "));
                                List.Write(Ztring(Text));
                                if (List.size()==1)
                                {
                                    //Trying with space
                                    List.Separator_Set(0, __T(","));
                                    List.Write(Ztring(Text));
                                }
                                if (List.size()==2)
                                {
                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                }
                            }
                        }
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "SignalRange"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "computer"))
                            Text = "Full";
                        Fill(Stream_Video, 0, Video_colour_range, Text);
                    }
                }
                if (!strcmp(ColorEncoding_Item->Name(), "WhitePoint"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        ZtringList List;
                        List.Separator_Set(0, __T(" "));
                        List.Write(Ztring(Text));
                        if (List.size()==1)
                        {
                            //Trying with space
                            List.Separator_Set(0, __T(","));
                            List.Write(Ztring(Text));
                        }
                        if (List.size()==2)
                        {
                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                        }
                    }
                }
            }
            Ztring colour_primaries;
            Get_MasteringDisplayColorVolume(colour_primaries, colour_primaries, Mastering); // Second part is not used
            if (!colour_primaries.empty())
                Fill(Stream_Video, 0, Video_colour_primaries, colour_primaries);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "EditRate"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
            {
                float32 FrameRate = Ztring().From_UTF8(Text).To_float32();
                if (FrameRate)
                {
                    Text = strchr(Text, ' ');
                    if (Text)
                    {
                        Text++;
                        float32 FrameRateD = Ztring().From_UTF8(Text).To_float32();
                        if (FrameRateD)
                            FrameRate /= FrameRateD;
                    }
                }
                Fill(Stream_Video, 0, Video_FrameRate, FrameRate);
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Level6"))
        {
            for (XMLElement* Level6_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Level6_Item; Level6_Item = Level6_Item->NextSiblingElement())
            {
                if (!strcmp(Level6_Item->Name(), "MaxCLL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if (Ztring().From_UTF8(Text).To_float32())
                            Fill(Stream_Video, 0, Video_MaxCLL, Ztring(Text) + __T(" cd/m2"));
                }
                if (!strcmp(Level6_Item->Name(), "MaxFALL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if (Ztring().From_UTF8(Text).To_float32())
                            Fill(Stream_Video, 0, Video_MaxFALL, Ztring(Text) + __T(" cd/m2"));
                }
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "PluginNode"))
        {
            for (XMLElement* PluginNode_Item=DolbyVisionGlobalData_Item->FirstChildElement(); PluginNode_Item; PluginNode_Item=PluginNode_Item->NextSiblingElement())
            {
                if (!strcmp(PluginNode_Item->Name(), "DolbyEDR") || !strcmp(PluginNode_Item->Name(), "DVGlobalData"))
                {
                    for (XMLElement* DolbyEDR_Item=PluginNode_Item->FirstChildElement(); DolbyEDR_Item; DolbyEDR_Item=DolbyEDR_Item->NextSiblingElement())
                    {
                        if (!strcmp(DolbyEDR_Item->Name(), "Characteristics"))
                        {
                            for (XMLElement* Characteristics_Item=DolbyEDR_Item->FirstChildElement(); Characteristics_Item; Characteristics_Item=Characteristics_Item->NextSiblingElement())
                            {
                                if (!strcmp(Characteristics_Item->Name(), "MasteringDisplay"))
                                {
                                    mastering_metadata_2086 Mastering;
                                    memset(&Mastering, 0xFF, sizeof(Mastering));
                                    for (XMLElement* MasteringDisplay_Item=Characteristics_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                                    {
                                        if (!strcmp(MasteringDisplay_Item->Name(), "MinimumBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "PeakBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "Primaries"))
                                        {
                                            for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                            {
                                                int8u i=(int8u)-1;
                                                if (!strcmp(Primaries_Item->Name(), "Green"))
                                                    i=0;
                                                if (!strcmp(Primaries_Item->Name(), "Blue"))
                                                    i=2;
                                                if (!strcmp(Primaries_Item->Name(), "Red"))
                                                    i=4;
                                                if (i!=(int8u)-1)
                                                {
                                                    if (const char* Text=Primaries_Item->GetText())
                                                    {
                                                        ZtringList List;
                                                        List.Separator_Set(0, __T(" "));
                                                        List.Write(Ztring(Text));
                                                        if (List.size()==1)
                                                        {
                                                            //Trying with space
                                                            List.Separator_Set(0, __T(","));
                                                            List.Write(Ztring(Text));
                                                        }
                                                        if (List.size()==2)
                                                        {
                                                            Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                            Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (!strcmp(MasteringDisplay_Item->Name(), "WhitePoint"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                    Ztring colour_primaries, luminance;
                                    Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                                    if (!colour_primaries.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                                    if (!luminance.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                                }
                            }
                        }
                        if (!strcmp(DolbyEDR_Item->Name(), "MasteringDisplay"))
                        {
                            mastering_metadata_2086 Mastering;
                            memset(&Mastering, 0xFF, sizeof(Mastering));
                            for (XMLElement* MasteringDisplay_Item=DolbyEDR_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                            {
                                if (!strcmp(MasteringDisplay_Item->Name(), "MinimumBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "PeakBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "Primaries"))
                                {
                                    for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                    {
                                        int8u i=(int8u)-1;
                                        if (!strcmp(Primaries_Item->Name(), "Green"))
                                            i=0;
                                        if (!strcmp(Primaries_Item->Name(), "Blue"))
                                            i=2;
                                        if (!strcmp(Primaries_Item->Name(), "Red"))
                                            i=4;
                                        if (i!=(int8u)-1)
                                        {
                                            if (const char* Text=Primaries_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                }
                                if (!strcmp(MasteringDisplay_Item->Name(), "WhitePoint"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                    {
                                        ZtringList List;
                                        List.Separator_Set(0, __T(" "));
                                        List.Write(Ztring(Text));
                                        if (List.size()==1)
                                        {
                                            //Trying with space
                                            List.Separator_Set(0, __T(","));
                                            List.Write(Ztring(Text));
                                        }
                                        if (List.size()==2)
                                        {
                                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                        }
                                    }
                                }
                            }
                            Ztring colour_primaries, luminance;
                            Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                            if (!colour_primaries.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                            if (!luminance.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                        }
                    }
                }
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Rate"))
        {
            int64u n = 0, d = 0;
            for (XMLElement* Rate_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Rate_Item; Rate_Item = Rate_Item->NextSiblingElement())
            {
                if (!strcmp(Rate_Item->Name(), "d"))
                    d = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
                if (!strcmp(Rate_Item->Name(), "n"))
                    n = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
            }
            if (n && d)
                Fill(Stream_Video, 0, Video_FrameRate, ((float32)n) / d);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "TrackName"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_Title, Text);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "UniqueID"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_UniqueID, Text);
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Version"))
        {
            if (const char* Text= DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Video, 0, Video_HDR_Format_Version, Text);
        }
    }
    }

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

} //NameSpace

#endif //defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)


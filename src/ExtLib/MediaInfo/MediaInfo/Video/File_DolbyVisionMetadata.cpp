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
#if defined(MEDIAINFO_MXF_YES)
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

    XMLElement* DolbyVisionGlobalData=document.FirstChildElement();
    if (!DolbyVisionGlobalData || strcmp(DolbyVisionGlobalData->Name(), "DolbyVisionGlobalData"))
    {
        Reject("DolbyVisionMetadata");
        return false;
    }

    Accept("DolbyVisionMetadata");
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, "HDR_Format", "Dolby Vision Metadata");
    if (const char* Text=DolbyVisionGlobalData->Attribute("version"))
        Fill(Stream_Video, 0, "HDR_Format_Version", Text);

    for (XMLElement* DolbyVisionGlobalData_Item=DolbyVisionGlobalData->FirstChildElement(); DolbyVisionGlobalData_Item; DolbyVisionGlobalData_Item=DolbyVisionGlobalData_Item->NextSiblingElement())
    {
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "ColorEncoding"))
        {
            mastering_metadata_2086 Mastering;
            memset(&Mastering, 0xFF, sizeof(Mastering));
            for (XMLElement* ColorEncoding_Item=DolbyVisionGlobalData_Item->FirstChildElement(); ColorEncoding_Item; ColorEncoding_Item=ColorEncoding_Item->NextSiblingElement())
            {
                if (!strcmp(ColorEncoding_Item->Name(), "Encoding"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "pq"))
                            Text="PQ";
                        Fill(Stream_Video, 0, "transfer_characteristics", Text);
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
                if (!strcmp(ColorEncoding_Item->Name(), "WhitePoint"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        ZtringList List;
                        List.Separator_Set(0, __T(","));
                        List.Write(Ztring(Text));
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
                Fill(Stream_Video, 0, "colour_primaries", colour_primaries);
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
                                                        List.Separator_Set(0, __T(","));
                                                        List.Write(Ztring(Text));
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
                                        Fill(Stream_Video, 0, "MasteringDisplay_ColorPrimaries", colour_primaries);
                                    if (!luminance.empty())
                                        Fill(Stream_Video, 0, "MasteringDisplay_Luminance", luminance);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Level6"))
        {
            for (XMLElement* Level6_Item=DolbyVisionGlobalData_Item->FirstChildElement(); Level6_Item; Level6_Item=Level6_Item->NextSiblingElement())
            {
                if (!strcmp(Level6_Item->Name(), "MaxCLL"))
                {
                    if (const char* Text=Level6_Item->GetText())
                        if (atof(Text))
                            Fill(Stream_Video, 0, "MaxCLL", Ztring(Text)+__T(" cd/m2"));
                }
                if (!strcmp(Level6_Item->Name(), "MaxFALL"))
                {
                    if (const char* Text=Level6_Item->GetText())
                        if (atof(Text))
                            Fill(Stream_Video, 0, "MaxFALL", Ztring(Text)+__T(" cd/m2"));
                }
            }
        }
        if (!strcmp(DolbyVisionGlobalData_Item->Name(), "Version"))
        {
            if (const char* Text= DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Video, 0, "HDR_Format_Version", Text);
        }
    }

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

} //NameSpace

#endif //MEDIAINFO_DCP_YES

